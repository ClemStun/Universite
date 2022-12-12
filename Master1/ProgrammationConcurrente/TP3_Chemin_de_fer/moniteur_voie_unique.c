#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <sens.h>
#include <train.h>
#include <moniteur_voie_unique.h>

/*---------- MONITEUR ----------*/

extern moniteur_voie_unique_t * moniteur_voie_unique_creer( const train_id_t nb )
{
  moniteur_voie_unique_t * moniteur = NULL ; 

  /* Creation structure moniteur */
  if( ( moniteur = malloc( sizeof(moniteur_voie_unique_t) ) ) == NULL  )
    {
      fprintf( stderr , "moniteur_voie_unique_creer: debordement memoire (%lu octets demandes)\n" , 
	       sizeof(moniteur_voie_unique_t) ) ;
      return(NULL) ; 
    }

  /* Creation de la voie */
  if( ( moniteur->voie_unique = voie_unique_creer() ) == NULL )
    return(NULL) ; 
  
  /* Initialisations du moniteur */

  printf("Initialisation des variables du moniteur\n");
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  moniteur->mutex = mutex;

  pthread_cond_init(&(moniteur->cond_e), NULL);
  pthread_cond_init(&(moniteur->cond_o), NULL);

  moniteur->EO = B_FALSE;
  moniteur->OE = B_FALSE;

  moniteur->nbAttenteE = 0;
  moniteur->nbAttenteO = 0;

  moniteur->nbTrainSurVoie = 0;
  moniteur->nbMaxTrain = nb;

  return(moniteur) ; 
}

extern int moniteur_voie_unique_detruire( moniteur_voie_unique_t ** moniteur )
{
  int noerr ; 

  /* Destructions des attribiuts du moniteur */

  pthread_mutex_destroy(&((*moniteur)->mutex));
  pthread_cond_destroy(&((*moniteur)->cond_e));
  pthread_cond_destroy(&((*moniteur)->cond_o));

  /* Destruction de la voie */
  if( ( noerr = voie_unique_detruire( &((*moniteur)->voie_unique) ) ) )
    return(noerr) ;

  /* Destruction de la strcuture du moniteur */
  free( (*moniteur) );

  (*moniteur) = NULL ; 

  return(0) ; 
}

extern void moniteur_voie_unique_entree_ouest( moniteur_voie_unique_t * moniteur ) 
{
  
  pthread_mutex_lock(&(moniteur->mutex));

  while(moniteur->EO == B_TRUE || moniteur->nbTrainSurVoie >= moniteur->nbMaxTrain){
    moniteur->nbAttenteO += 1;
    pthread_cond_wait(&(moniteur->cond_o), &(moniteur->mutex));
    moniteur->nbAttenteO -= 1;
  }

  if(moniteur->nbTrainSurVoie == 0){
    moniteur->OE = B_TRUE;
  }

  moniteur->nbTrainSurVoie += 1;

  pthread_cond_signal(&(moniteur->cond_o));

  pthread_mutex_unlock(&(moniteur->mutex));

}

extern void moniteur_voie_unique_sortie_est( moniteur_voie_unique_t * moniteur ) 
{
  
  pthread_mutex_lock(&(moniteur->mutex));
  
  moniteur->nbTrainSurVoie -= 1;

  if(moniteur->nbTrainSurVoie == 0){

    moniteur->OE = B_FALSE;

    if(moniteur->nbAttenteE > 0){
      pthread_cond_signal(&(moniteur->cond_e));
    }else if(moniteur->nbAttenteO > 0){
      pthread_cond_signal(&(moniteur->cond_o));
    }

  }else if(moniteur->nbTrainSurVoie > 0 && moniteur->nbAttenteO > 0){
    pthread_cond_signal(&(moniteur->cond_o));
  }

  pthread_mutex_unlock(&(moniteur->mutex));

}

extern void moniteur_voie_unique_entree_est( moniteur_voie_unique_t * moniteur ) 
{
  pthread_mutex_lock(&(moniteur->mutex));

  while(moniteur->OE == B_TRUE || moniteur->nbTrainSurVoie >= moniteur->nbMaxTrain){
    moniteur->nbAttenteE += 1;
    pthread_cond_wait(&(moniteur->cond_e), &(moniteur->mutex));
    moniteur->nbAttenteE -= 1;
  }

  if(moniteur->nbTrainSurVoie == 0){
    moniteur->EO = B_TRUE;
  }

  moniteur->nbTrainSurVoie += 1;

  pthread_cond_signal(&(moniteur->cond_e));

  pthread_mutex_unlock(&(moniteur->mutex));
}

extern void moniteur_voie_unique_sortie_ouest( moniteur_voie_unique_t * moniteur ) 
{
  pthread_mutex_lock(&(moniteur->mutex));
  
  moniteur->nbTrainSurVoie -= 1;

  if(moniteur->nbTrainSurVoie == 0){

    moniteur->EO = B_FALSE;

    if(moniteur->nbAttenteO > 0){
      pthread_cond_signal(&(moniteur->cond_o));
    }else if(moniteur->nbAttenteE > 0){
      pthread_cond_signal(&(moniteur->cond_e));
    }

  }else if(moniteur->nbTrainSurVoie > 0 && moniteur->nbAttenteE > 0){
    pthread_cond_signal(&(moniteur->cond_e));
  }

  pthread_mutex_unlock(&(moniteur->mutex));
}

/*
 * Fonctions set/get 
 */

extern 
voie_unique_t * moniteur_voie_unique_get( moniteur_voie_unique_t * const moniteur )
{
  return( moniteur->voie_unique ) ; 
}


extern 
train_id_t moniteur_max_trains_get( moniteur_voie_unique_t * const moniteur )
{
  return( moniteur->nbMaxTrain ) ; 
}

/*
 * Fonction de deplacement d'un train 
 */

extern
int moniteur_voie_unique_extraire( moniteur_voie_unique_t * moniteur , train_t * train , zone_t zone  ) 
{
  return( voie_unique_extraire( moniteur->voie_unique, 
				(*train), 
				zone , 
				train_sens_get(train) ) ) ; 
}

extern
int moniteur_voie_unique_inserer( moniteur_voie_unique_t * moniteur , train_t * train , zone_t zone ) 
{ 
  return( voie_unique_inserer( moniteur->voie_unique, 
			       (*train), 
			       zone, 
			       train_sens_get(train) ) ) ;
}
