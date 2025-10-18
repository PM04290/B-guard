// ============ CONFIGURATION ============
#define BUFFER_SIZE 60          // Buffer de 60 minutes

// Seuils initiaux (seront auto-calibrés)
float seuil_dynamique_jour = 3000;
float seuil_dynamique_nuit = 800;
float seuil_montee = 50;        // Variation/min pour détecter montée
float seuil_descente = 50;      // Variation/min pour détecter descente

// ============ VARIABLES GLOBALES ============
enum Phase { NUIT, AUBE, JOUR, CREPUSCULE };
Phase phase_actuelle = NUIT;
Phase phase_precedente = NUIT;

// Timer logiciel
unsigned long dernier_millis = 0;
uint32_t secondes = 0;
uint16_t minutes = 0;
uint8_t heures = 0;

// Buffer circulaire de luminosité
uint16_t buffer_ldr[BUFFER_SIZE];
uint8_t index_buffer = 0;
bool buffer_plein = false;

// Gestion des mesures quotidiennes
uint8_t mesures_faites_aujourd_hui = 0;
float max_observe_depuis_aube = 0;

// Calibration
uint16_t max_24h = 0;
uint16_t min_24h = 4095;
uint16_t compteur_calibration = 0;

// ============ FONCTIONS ============
void cycleInit()
{
  for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
    buffer_ldr[i] = 0;
  }
}

// Calculer moyenne sur N dernières minutes
float calculer_moyenne(uint8_t nb_minutes) {
  if (nb_minutes > BUFFER_SIZE) nb_minutes = BUFFER_SIZE;
  if (!buffer_plein && index_buffer < nb_minutes) {
    nb_minutes = index_buffer;
  }
  if (nb_minutes == 0) return 0;

  uint32_t somme = 0;
  for (uint8_t i = 0; i < nb_minutes; i++) {
    uint8_t idx = (index_buffer - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
    somme += buffer_ldr[idx];
  }
  return (float)somme / nb_minutes;
}

// Détecter la phase du jour
void detecter_phase(float moy_5min, float moy_15min, float tendance) {
  phase_precedente = phase_actuelle;

  if (moy_5min < seuil_dynamique_nuit) {
    phase_actuelle = NUIT;
  }
  else if (tendance > seuil_montee && moy_5min > seuil_dynamique_nuit && moy_5min < seuil_dynamique_jour) {
    phase_actuelle = AUBE;
  }
  else if (moy_5min > seuil_dynamique_jour && abs(tendance) < seuil_montee * 0.5) {
    phase_actuelle = JOUR;
  }
  else if (tendance < -seuil_descente && moy_5min > seuil_dynamique_nuit) {
    phase_actuelle = CREPUSCULE;
  }
}

// Déclencher une mesure
void declencher_mesure(const char* moment) {
  Serial.println("\n========================================");
  Serial.print("*** MESURE DECLENCHEE : ");
  Serial.print(moment);
  Serial.println(" ***");
  Serial.printf("Heure virtuelle : %02d:%02d:%02d\n", heures, minutes, secondes);
  Serial.printf("Luminosite actuelle : %d\n", buffer_ldr[(index_buffer - 1 + BUFFER_SIZE) % BUFFER_SIZE]);
  Serial.println("========================================\n");

  // === VOTRE CODE DE MESURE ICI ===
  // Ex: lire capteur, logger, envoyer LoRa, etc.
  // ================================

  mesures_faites_aujourd_hui++;
}

// Vérifier si une mesure doit être déclenchée
void verifier_declenchement_mesure(float moy_5min, float tendance) {
  if (mesures_faites_aujourd_hui >= 3) return;

  // MESURE 1 : AUBE (transition NUIT → AUBE)
  if (phase_actuelle == AUBE && phase_precedente == NUIT && mesures_faites_aujourd_hui == 0) {
    if (tendance > seuil_montee * 1.5 && heures >= 4 && heures <= 10) {
      declencher_mesure("AUBE");
      max_observe_depuis_aube = moy_5min;
    }
  }

  // MESURE 2 : MIDI (pic de luminosité)
  if (phase_actuelle == JOUR && mesures_faites_aujourd_hui == 1) {
    if (moy_5min > max_observe_depuis_aube) {
      max_observe_depuis_aube = moy_5min;
    }
    // Attendre une baisse significative après le pic
    if (moy_5min < max_observe_depuis_aube * 0.95 && heures >= 11 && heures <= 15) {
      declencher_mesure("MIDI");
      max_observe_depuis_aube = 0;
    }
  }

  // MESURE 3 : CREPUSCULE (transition JOUR → CREPUSCULE)
  if (phase_actuelle == CREPUSCULE && phase_precedente == JOUR && mesures_faites_aujourd_hui == 2) {
    if (tendance < -seuil_descente * 1.5 && heures >= 16 && heures <= 22) {
      declencher_mesure("CREPUSCULE");
    }
  }
}

// Auto-calibration des seuils
void auto_calibration() {
  if (compteur_calibration >= 1440) { // 24h = 1440 minutes
    float range = max_24h - min_24h;

    seuil_dynamique_jour = min_24h + range * 0.70;
    seuil_dynamique_nuit = min_24h + range * 0.20;
    seuil_montee = range * 0.03;  // 3% du range
    seuil_descente = range * 0.03;

    Serial.println("\n=== AUTO-CALIBRATION ===");
    Serial.printf("Min 24h: %d, Max 24h: %d\n", min_24h, max_24h);
    Serial.printf("Seuil jour: %.0f, Seuil nuit: %.0f\n", seuil_dynamique_jour, seuil_dynamique_nuit);
    Serial.printf("Seuil montee/descente: %.1f\n", seuil_montee);
    Serial.println("========================\n");

    // Reset pour nouvelle période
    max_24h = 0;
    min_24h = 4095;
    compteur_calibration = 0;
  }
}

// Traiter chaque nouvelle minute
void traiter_nouvelle_minute() {
  // Lire LDR
  uint16_t lux = Lumi->getFloat();

  // Stocker dans buffer circulaire
  buffer_ldr[index_buffer] = lux;
  index_buffer = (index_buffer + 1) % BUFFER_SIZE;
  if (index_buffer == 0) buffer_plein = true;

  // Mise à jour min/max pour calibration
  if (lux > max_24h) max_24h = lux;
  if (lux < min_24h) min_24h = lux;
  compteur_calibration++;

  // Calculer moyennes glissantes
  float moy_5min = calculer_moyenne(5);
  float moy_15min = calculer_moyenne(15);

  // Calculer tendance (variation par minute)
  float tendance = 0;
  if (moy_15min > 0) {
    tendance = (moy_5min - moy_15min) / 10.0;
  }

  // Détection de phase
  detecter_phase(moy_5min, moy_15min, tendance);

  // Debug toutes les 5 minutes
  if (minutes % 5 == 0) {
    Serial.printf("[%02d:%02d] Lux: %4d | Moy5: %4.0f | Tend: %+6.1f | Phase: ",
                  heures, minutes, lux, moy_5min, tendance);
    switch (phase_actuelle) {
      case NUIT: Serial.print("NUIT"); break;
      case AUBE: Serial.print("AUBE"); break;
      case JOUR: Serial.print("JOUR"); break;
      case CREPUSCULE: Serial.print("CREPUSCULE"); break;
    }
    Serial.printf(" | Mesures: %d/3\n", mesures_faites_aujourd_hui);
  }

  // Vérifier déclenchement mesure
  verifier_declenchement_mesure(moy_5min, tendance);

  // Auto-calibration
  auto_calibration();
}
