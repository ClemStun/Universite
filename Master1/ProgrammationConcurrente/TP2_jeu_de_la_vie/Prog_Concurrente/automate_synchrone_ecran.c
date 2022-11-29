#include <getopt.h>
#include <pthread.h>
#include <signal.h>

#include <automate_ecran.h>
#include <coords_stdio.h>

/* Argumtent de la fonction calcule_gen_suiv */
typedef struct args_s{

  int i; //ordonnée de la cellule qui évolue
  int j; //abscisse de la cellule qui évolue
  automate_t * automate; //pointeur sur l'automate -- variable critique commun à tout les threads --
  int * n; //pointeur sur le numéro de la génération en cours -- variable critique commun à tout les threads --
  int nb_generations; //nombre de génération
  cellule_regles_t regles; //regles de l'automate
  pthread_mutex_t * mutex; //mutex des threads
  pthread_barrier_t * barriere; //barrière d'attente pour les threads

} args_t;

typedef struct args_c_s{

  automate_t * automate;
  int * n; //pointeur sur le numéro de la génération en cours -- variable critique commun à tout les threads --
  int nb_generations; //nombre de génération
  pthread_mutex_t * mutex; //mutex des threads
  pthread_barrier_t * barriere; //barrière d'attente pour les threads

} args_c_t;

/*!
 * \file automate_synchrone_ecran.c
 * \brief Exécution d'un Automate Synchrone avec sortie sur un écran géré par <code>ncurses</code>
 * Les cellules sont gérées indépendamment par des threads
 */

ecran_t * Ecran = NULL ;

/* \brief Handler de capture du signal d'arrêt SIGINT */
static
void hand_arret( int sig ) 
{
  ecran_detruire(&Ecran) ; 
  exit(0); 
}

/**
 * Calcule la génération suivante
 * les variables critiques sont uniquement gérer à l'intérieur de la section critique
 *
 * \param args arguments de la fonction
 */
void calcule_gen_suiv(void * arguments){

  /* Récupération des arguments */
  args_t * args = (args_t *)arguments;

  booleen_t fini = FAUX;
  int noerr = CORRECT;

  while(!fini){

    usleep((random()%10000)+10);

    /* Entrée section critique */
    pthread_mutex_lock(args->mutex);

    if(*(args->n) <= args->nb_generations){

      if((noerr = automate_cellule_evoluer(args->automate, automate_get(args->automate, args->i, args->j), &(args->regles)))){
        fprintf(stderr, "%s : Pb evolution  cellule [%d,%d], sortie erreur %d\n", "Thread", args->i, args->j, noerr);
	      err_print(noerr); 
        pthread_mutex_unlock(args->mutex);
        pthread_exit(0); 
      }

    }else{
      fini = VRAI;
    }

    pthread_mutex_unlock(args->mutex);
    /* Sortie section critique */
    pthread_barrier_wait(args->barriere);

  }

  pthread_exit(NULL);

}

void passage_gen(void * arguments){

  args_c_t * args = (args_c_t *)arguments;

  int noerr = CORRECT;
  char mess[STRING_LG_MAX];

  while((*args->n) <= args->nb_generations){

    pthread_barrier_wait(args->barriere);

    pthread_mutex_lock(args->mutex);

    /* Affichage */
    automate_wprint(Ecran, args->automate);
    sprintf(mess, "SYNC %d", *(args->n));
    ecran_message_afficher(Ecran, mess);
    usleep(100000);

    /* Passage à la prochaine génération (une seule cellule change) */
    if((noerr = automate_generer(args->automate))){
      fprintf(stderr, "%s : Pb prise en compte nouvelle generation automate, sortie erreur %d\n", "Thread", noerr);
      err_print(noerr);  
      pthread_mutex_unlock(args->mutex);
      pthread_exit(0); 
    }

    (*args->n)++;

    pthread_mutex_unlock(args->mutex);

  }

  pthread_barrier_wait(args->barriere);
  pthread_exit(NULL);

}

/*
 * Verifie si toutes les coordonnées d'une liste appartiennent à un automate
 */
static
int verifier_cellules( const int hauteur , const int largeur , const coords_t * const liste_coords )
{
  coord_t coord ;
  int i , l , c ;
  int nb = 0 ;

  if( liste_coords == NULL )
    return(CORRECT) ;

  nb = coords_nb_get(liste_coords) ;
  
  for(i=0;i<nb;i++)
    {
      coord = coords_get( liste_coords , i ) ;
      l = coord_lig_get(&coord) ;
      c = coord_col_get(&coord) ;
      if( (l<0) || (l>= hauteur) || (c<0) || (c>=largeur) )
	{
	  fprintf( stderr ,"verifier_cellules : la coordonnées [%d,%d] n'appartient pas à l'automate de dim %dX%d\n" , l, c , hauteur , largeur ) ;
	  return(ERR_DIM) ;
	}
    }
  
  return(CORRECT) ; 
}


/*
 * Programme Principal 
 */

static
void usage( char * nomprog ) 
{
  printf( "usage: %s [options] <hauteur> <largeur> <nb generations>\n" , nomprog ) ; 
  printf( "options:\n" ) ; 
  printf( "\t--verbose : trace des principales actions (defaut = desactive)\n" ) ; 
  printf( "\t--help    : affiche cette aide\n" ) ; 
  printf( "\t--fichier <fichier initialisation> : initialisation de l'automate avec un fichier\n" ) ;
  printf( "\t--case <l,c> : initialisation de la case [l,c] de l'automate avec une cellule vivante\n" ) ;
  printf( "\t--naissance <nb> : nombre de voisins pour la naissance d'une cellule (defaut %d)\n" , NAISSANCE ) ;
  printf( "\t--isolement <nb> : nombre de voisins minimum en dessous duquel la cellule meurt isolee (defaut %d)\n" , ISOLEMENT ) ;
  printf( "\t--etouffement <nb> : nombre de voisins maximum au dessus duquel la cellule meurt etouffee (defaut %d)\n" , ETOUFFEMENT ) ; 
}

static struct option longopts[] =
  {
    {"verbose", no_argument, NULL, (int)'v'},
    {"help", no_argument, NULL, (int)'h'},
    {"fichier", required_argument, NULL, (int)'f'},
    {"case", required_argument, NULL, (int)'c'},
    {"naissance", required_argument, NULL, (int)'n'},
    {"isolement", required_argument, NULL, (int)'i'},
    {"etouffement", required_argument, NULL, (int)'e'},
    {0, 0, 0, 0}
  };

int 
main( int argc , char * argv[] )
{
  int noerr = CORRECT ;

   /* --- Gestion des options --- */
  int opt ; 
  char nomprog[STRING_LG_MAX] ;
  booleen_t  verbose = FAUX ;
  
  /* --- Options --- */
  char * fichier_init = NULL ;
  coords_t * coords_init = coords_new() ; 

  /* --- Automate --- */
  automate_t * automate = NULL ; 
  cellule_regles_t regles = cellule_regles_default() ;
  
  /*----------*/

  /*  Captures des options et des paramètres */
  
  /* Section options */
  strcpy( nomprog , argv[0] ) ;
  while ((opt = getopt_long(argc, argv, "vhf:c:n:i:e:", longopts, NULL)) != -1 )
    {
      switch(opt)
	{
	case 'c' : /* --case d'initialisation */
	  {
	    int l , c ;
	    coord_t coord ;
	    sscanf( optarg , "%d,%d" , &l , &c ) ;
	    coord_set( &coord , l , c ) ; 
	    
	    if( (noerr = coords_add( coords_init , coord ) ) )
	      {
		fprintf( stderr , "%s: pb ajout case %d,%d dans liste des coordonnées d'initialisation, sortie erreur %d\n" , nomprog , l , c , noerr ) ;
		err_print(noerr) ;
		exit(0) ; 
	      }
	    break ;
	  }
	  
	case 'f':  /* --fichier d'initialisation */
	  fichier_init = malloc( strlen(optarg)+1 ) ; 
	  strcpy(fichier_init,optarg) ;
	  break;
	  
	case 'n':  /* --naissance */
	  cellule_regles_naissance_set(&regles, atoi(optarg) ) ; 
	  break;

	case 'i':  /* --isolement */
	  cellule_regles_isolement_set(&regles, atoi(optarg) ) ; 
	  break;

	case 'e':  /* --etouffement */
	  cellule_regles_etouffement_set(&regles, atoi(optarg) ) ; 
	  break;
	  
	case 'v' :  /* --verbose */
	  verbose = VRAI ;
	  break ; 

	case 'h': /* --help */
	  usage(nomprog) ;
	  exit(0);
	  break;

	case '?':
	  usage(nomprog) ;
	  exit(0);
	  break;

	default:
	  usage(nomprog) ;
	  exit(0);
	  break;
	}
    }
  argc -= optind ;
  argv += optind ;

  /* --- Parametres ---*/
  /* argc == nb de parametres sans le nom de la commande */
  /* argv[0] --> 1er parametre */
  
  if( argc != 3 )
    {
      printf("%s : Nombre de parametres incorrect : %d (3 attendus)\n\n" , nomprog , argc ) ; 
      usage(nomprog) ; 
      exit(-1) ;
    }

  int hauteur = atoi(argv[0]) ;
  int largeur = atoi(argv[1]) ;
  int nb_generations = atoi(argv[2]) ; 

  /* ---------- */
  
  srandom( (unsigned int)getpid() );

  signal( SIGINT , hand_arret ) ;

  if( verbose ) printf("\n----- Debut %s  Automate %dX%d %d generations -----\n" ,
		       nomprog , hauteur , largeur , nb_generations  );
  
  /*  Création de l'automate avec les coordonnées des cellules vivantes entrées dans les options -c et -f */
  
  if( verbose ) printf("\n----- Debut %s -----\n" , nomprog  );

  /* Creation d'un automate */
  if( verbose ) printf("Lecture des coordonnees initiales\n");
  if( (noerr = coords_scan( fichier_init , coords_init )) )
    {
      fprintf( stderr ,"%s : Pb chargement liste des coords initiales a partir de %s\n" , nomprog , fichier_init ) ;
      err_print(noerr) ; exit(0) ; 
    }
  if( verbose ) coords_print( stdout , coords_init ) ;

  if( verbose ) printf("Vérification des coordonnées des cellules intiales\n") ;
  if( (noerr = verifier_cellules( hauteur , largeur , coords_init ) ) )
    {
      fprintf( stderr ,"%s : Coordonnées initiales incorrectes, sortie erreur %d\n" , nomprog , noerr ) ;
      err_print(noerr) ; exit(0) ; 
    }
  
  if( verbose ) printf("Creation automate\n") ; 
  if( (automate = automate_new( hauteur , largeur , coords_init )) == NULL )
    {
      fprintf( stderr , "%s : Erreur creation automate\n" , nomprog );
      exit(-1);
    }

  /*  Création de l'écran pour affichage de cet automate */
  if( verbose ) printf("Debut Creation ecran\n");
  if( (Ecran = automate_ecran_creer(automate))  == ECRAN_NULL ) 
    {
      fprintf( stderr , "%s : Erreur creation ecran\n" , nomprog );
      exit(-1);
    }

  /* ----- */

  automate_wprint(Ecran, automate);

  /********************************/
  /* Gestion des cellules A FAIRE */
  /********************************/

  int i, j;
  pthread_t threads_id[hauteur][largeur];
  pthread_t thread_controle;

  pthread_attr_t attr;

  args_t args[hauteur][largeur];
  args_c_t args_controle;

  /* Mutex */
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  /* Barriere */
  pthread_barrier_t barriere;
  pthread_barrier_init(&barriere, NULL, hauteur*largeur+1);

  /* Variable critique */
  int n = 1;

  /* arguments */
  args_t args_p;
  args_p.automate = automate;
  args_p.n = &n;
  args_p.nb_generations = nb_generations;
  args_p.regles = regles;
  args_p.mutex = &mutex;
  args_p.barriere = &barriere;

  pthread_attr_init(&attr);
  /* Valeur par défaut : pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS); */

  /* Lancement d'un thread par cellule */
  for(i = 0; i < hauteur; i++){
    for(j = 0; j < largeur; j++){

        args_p.i = i;
        args_p.j = j;
        args[i][j] = args_p;
        pthread_create(&threads_id[i][j], &attr, (void *)calcule_gen_suiv, &args[i][j]);

    }
  }
  
  /* Thread de controle des générations */
  args_controle.n = &n;
  args_controle.nb_generations = nb_generations;
  args_controle.mutex = &mutex;
  args_controle.barriere = &barriere;
  args_controle.automate = automate;

  pthread_create(&thread_controle, &attr, (void *)passage_gen, &args_controle);

  /* Attente de la fin de tous les threads */
  for(i = 0; i < hauteur; i++){
    for(j = 0; j < largeur; j++){

      pthread_join(threads_id[i][j], NULL);

    }
  }

  pthread_join(thread_controle, NULL);

  pthread_mutex_destroy(&mutex);
  pthread_barrier_destroy(&barriere);
  
  ecran_message_stop_afficher( Ecran , "<Entree>" ) ;
    
  /* ---------- */
  
  /*  Destruction de l'écran  */
  if( (noerr = ecran_detruire(&Ecran) ) )
    {
      fprintf( stderr ,"%s : Pb destruction ecran, sortie erreur %d\n" , nomprog , noerr ) ;
      err_print(noerr) ;
      exit(0) ; 
    }

  /*  Destruction des variables: liste des coordonnées initiales, des cellules et de l'automate */
  if( verbose ) printf("Destruction liste de coordonnées d'initialisation\n") ;
  if( ( noerr = coords_destroy( &coords_init ) ) )
    {
      fprintf( stderr ,"%s : Pb destruction des coords d'initialisation, sortie erreur %d\n" , nomprog , noerr ) ;
      err_print(noerr) ;
      exit(0) ; 
    }

  if( verbose ) printf("Destruction automate\n") ; 
  if( (noerr = automate_destroy( &automate ) ) )
    {
      fprintf( stderr , "%s : sortie erreur %d\n" , nomprog , noerr ) ;
      err_print(noerr) ;
      exit(0) ;  
    }
 
  if( verbose ) printf("\n------ Fin %s -----\n\n" , nomprog );

  exit(0);
}
