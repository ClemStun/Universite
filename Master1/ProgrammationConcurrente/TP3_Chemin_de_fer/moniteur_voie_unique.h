#ifndef _MONITEUR_VOIE_UNIQUE_H_
#define _MONITEUR_VOIE_UNIQUE_H_

#include <voie_unique.h>
#include <pthread.h>

typedef struct moniteur_voie_unique_s
{
  /* Variables Gestion moniteur */
  boolean_t OE; //Ouest vers Est
  boolean_t EO; //Est vers Ouest

  int nbAttenteO; //Train en attente entrée Ouest
  int nbAttenteE; //Train en attente entrée Est

  int nbTrainSurVoie; //Nombre de train sur la portion de voie unique
  int nbMaxTrain;

  pthread_mutex_t mutex;
  pthread_cond_t cond_o; //Condition entrée Ouest
  pthread_cond_t cond_e; //Condition entrée Est

  /* Attributs */
  voie_unique_t * voie_unique ;		/* section de a voie unique */
} moniteur_voie_unique_t ;

#define MAX_MONITEURS_VOIES_UNIQUES 5
/* Compteur/Index de moniteurs de voies uniques */
typedef int moniteur_voie_unique_id_t ;

/* Creation d'un moniteur */
extern moniteur_voie_unique_t * moniteur_voie_unique_creer( const train_id_t nb ) ;
/* Destruction d'un moniteur */
extern int moniteur_voie_unique_detruire( moniteur_voie_unique_t ** moniteur );

/* Pour les trains allant dans le sens Ouest --> Est */ 
/*    - Procedure a executer avant d'entrer dans la voie unique */
extern void moniteur_voie_unique_entree_ouest( moniteur_voie_unique_t * moniteur ) ;
/*    - Procedure a executer apres etre sorti de la voie unique */
extern void moniteur_voie_unique_sortie_ouest( moniteur_voie_unique_t * moniteur ) ;

/* Pour les trains allant dans le sens Est --> Ouest */ 
/*    - Procedure a executer avant d'entrer dans la voie unique */
extern void moniteur_voie_unique_entree_est( moniteur_voie_unique_t * moniteur ) ;
/*    - Procedure a executer apres etre sorti de la voie unique */
extern void moniteur_voie_unique_sortie_est( moniteur_voie_unique_t * moniteur ) ;


/* Fonctions set/get */
extern voie_unique_t * moniteur_voie_unique_get( moniteur_voie_unique_t * const moniteur ) ;
extern train_id_t moniteur_max_trains_get( moniteur_voie_unique_t * const moniteur );


/* Fonction de deplacement d'un train */
extern int moniteur_voie_unique_extraire( moniteur_voie_unique_t * moniteur , train_t * train , zone_t zone  ) ;
extern int moniteur_voie_unique_inserer(  moniteur_voie_unique_t * moniteur , train_t * train , zone_t zone )  ; 

#endif
