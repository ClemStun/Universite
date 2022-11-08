#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <commun.h>
#include <liste.h>
#include <piste.h>

#define P -1
#define V 1

/**
 * Prend le sémaphore (P()) numéro 'num_sem' dans la liste de sémaphore 'id_sem'
 *
 * @param int id_sem id de la liste des sémaphore
 * @param int num_sem numéro du sémaphore sur lequel effectuer l'opération
 *
 * @return Arrete le processus en renvoyant un message d'erreur 
 */
void prendreSemaphore(int id_sem, int num_sem){

  struct sembuf buffer;
  buffer.sem_op = P;
  buffer.sem_num = num_sem;
  buffer.sem_flg = 0;

  if(semop(id_sem, &buffer, 1) == -1){
    printf("Erreur lors de l'opération P sur le sémaphore %d\n", id_sem);
    exit(-1);
  }

}

/**
 * Rend le sémaphore (V()) numéro 'num_sem' dans la liste de sémaphore 'id_sem'
 *
 * @param int id_sem id de la liste des sémaphore
 * @param int num_sem numéro du sémaphore sur lequel effectuer l'opération
 *
 * @return Arrete le processus en renvoyant un message d'erreur 
 */
void rendreSemaphore(int id_sem, int num_sem){

  struct sembuf buffer;
  buffer.sem_op = V;
  buffer.sem_num = num_sem;
  buffer.sem_flg = 0;

  if(semop(id_sem, &buffer, 1) == -1){
    printf("Erreur lors de l'opération V sur le sémaphore %d\n", id_sem);
    exit(-1);
  }

}

int 
main( int nb_arg , char * tab_arg[] )
{     

  int cle_piste ;
  piste_t * piste = NULL ;

  int cle_liste ;
  liste_t * liste = NULL ;

  char marque ;

  booleen_t fini = FAUX ;
  booleen_t isDecanille = FAUX;
  piste_id_t deplacement = 0 ;
  piste_id_t depart = 0 ;
  piste_id_t arrivee = 0 ;
  
  cell_t cell_cheval ;

  elem_t elem_cheval ;

  int ind_elem; //indice des éléments rechercher dans la liste

  /*-----*/

  if( nb_arg != 4 )
    {
      fprintf( stderr, "usage : %s <cle de piste> <cle de liste> <marque>\n" , tab_arg[0] );
      exit(-1);
    }
  
  if( sscanf( tab_arg[1] , "%d" , &cle_piste) != 1 )
    {
      fprintf( stderr, "%s : erreur , mauvaise cle de piste (%s)\n" , 
	       tab_arg[0]  , tab_arg[1] );
      exit(-2);
    }


  if( sscanf( tab_arg[2] , "%d" , &cle_liste) != 1 )
    {
      fprintf( stderr, "%s : erreur , mauvaise cle de liste (%s)\n" , 
	       tab_arg[0]  , tab_arg[2] );
      exit(-2);
    }

  if( sscanf( tab_arg[3] , "%c" , &marque) != 1 )
    {
      fprintf( stderr, "%s : erreur , mauvaise marque de cheval (%s)\n" , 
	       tab_arg[0]  , tab_arg[3] );
      exit(-2);
    }
     

  /* Init de l'attente */
  commun_initialiser_attentes() ;


  /* Init de la cellule du cheval pour faire la course */
  cell_pid_affecter( &cell_cheval  , getpid());
  cell_marque_affecter( &cell_cheval , marque );

  /* Init de l'element du cheval pour l'enregistrement */
  elem_cell_affecter(&elem_cheval , cell_cheval ) ;
  elem_etat_affecter(&elem_cheval , EN_COURSE ) ;

  //Récupération de la piste
  piste_id_t shmid_piste = shmget(cle_piste, sizeof(piste_t), 0666);
  if((piste = shmat(shmid_piste, NULL, 0)) == -1){
    printf("Erreur lors de la récupération de la piste");
    exit(-1);
  }

  //Récupération de la liste
  liste_id_t shmid_liste = shmget(cle_liste, sizeof(liste_t), 0666);
  if((liste = shmat(shmid_liste, NULL, 0)) == -1){
    printf("Erreur lors de la récupération de la liste");
    exit(-1);
  }

  //Récupération de l'id du sémaphore de la piste
  piste_id_t id_piste;

  if((id_piste = semget(cle_piste, PISTE_LONGUEUR, 0666)) == -1){
    printf("Erreur lors de la récupération du sémaphore de la piste");
    exit(-1);
  }

  liste_id_t id_liste;

  //Récupération de l'id du sémaphore de la liste
  if((id_liste = semget(cle_liste, 1, 0666)) == -1){
    printf("Erreur lors de la récupération du sémaphore de la liste");
    exit(-1);
  }

  /* 
   * Enregistrement du cheval dans la liste
   */

  prendreSemaphore(id_liste, 0);

  liste_elem_ajouter(liste, elem_cheval);

  rendreSemaphore(id_liste, 0);

  while( ! fini )
    {
      /* Attente entre 2 coup de de */
      commun_attendre_tour() ;

      //Récupération du sémaphore de la case de départ du cheval
      prendreSemaphore(id_piste, depart);

      /* 
       * Verif si pas decanille 
       */

      /*
       * On prend le sémaphore de la liste pour éviter la supression d'un cheval
       * pendant la lecture des indice
       */

      prendreSemaphore(id_liste, 0);

      liste_elem_rechercher(&ind_elem, liste, elem_cheval);

      if(elem_decanille(liste_elem_lire(liste, ind_elem))){
        fini = VRAI;
        isDecanille = VRAI;
        //Si on est décanillé la cellule de la piste a déjà été remplacer
      }

      rendreSemaphore(id_liste, 0);

      if(fini != VRAI){

        /*
         * Avancee sur la piste
         */


        /* Coup de de */
        deplacement = commun_coup_de_de() ;
  #ifdef _DEBUG_
        printf(" Lancement du De --> %d\n", deplacement );
  #endif

        arrivee = depart+deplacement ;

        if( arrivee > PISTE_LONGUEUR-1 ){
          arrivee = PISTE_LONGUEUR-1 ;
          fini = VRAI ;
          piste_cell_effacer(piste, depart); //On retire le cheval de la piste (il a fini)
        }

        if( depart != arrivee && fini == FAUX){

          /*
           * On prend le sémaphore de la case d'arriver pour être sur que si un cheval se trouve il ne joue pas pendant qu'on le décanille
           * Ou le décanille pendant qu'il joue
           */
          
          prendreSemaphore(id_piste, arrivee);

          /* 
           * Si case d'arrivee occupee alors on decanille le cheval existant 
           */

          cell_t cell_arrivee;
          elem_t cheval_dec;

          piste_cell_lire(piste, arrivee, &cell_arrivee);


          if(piste_cell_occupee(piste, arrivee) == VRAI){

            prendreSemaphore(id_liste, 0);

            elem_cell_affecter(&cheval_dec, cell_arrivee);
            liste_elem_rechercher(&ind_elem, liste, cheval_dec);
            liste_elem_decaniller(liste, ind_elem);

            rendreSemaphore(id_liste, 0);

          }          

          /* 
           * Deplacement: effacement case de depart, affectation case d'arrivee 
           */
          
          piste_cell_effacer(piste, depart);
          piste_cell_affecter(piste, arrivee, elem_cheval.cell);

    #ifdef _DEBUG_
        printf("Deplacement du cheval \"%c\" de %d a %d\n",
        marque, depart, arrivee );
    #endif

          /*
          * Fin du tour on libère le sémaphore de la case d'arrivée
          * et la case de départ
          */
          rendreSemaphore(id_piste, arrivee);
        }

        /* Affichage de la piste  */
        piste_afficher_lig( piste );

      }

      rendreSemaphore(id_piste, depart);
	  
      depart = arrivee ;

	}

  if(!isDecanille)
    printf( "Le cheval \"%c\" A FRANCHIT LA LIGNE D ARRIVEE\n" , marque );
  else
    printf("Le cheval \"%c\" EST DECANILLE\n", marque);

 
     
  prendreSemaphore(id_liste, 0);

  liste_elem_rechercher(&ind_elem, liste, elem_cheval);
  liste_elem_supprimer(liste, ind_elem);

  rendreSemaphore(id_liste, 0);
  
  exit(0);
}
