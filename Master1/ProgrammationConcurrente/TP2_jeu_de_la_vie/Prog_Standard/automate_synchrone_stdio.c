#include <string.h>
#include <getopt.h>

#include <automate_stdio.h>
#include <coords_stdio.h>

/*!
 * \file automate_synchrone_stdio.c
 * \brief Exécution d'un Automate Synchrone sur sortie standard
 */

/*!
 * Verifie si toutes les coordonnées d'une liste appartiennent à un automate
 */
static
int verifier_cellules( const int hauteur, const int largeur , const coords_t * const liste_coords )
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
  printf( "\t--verbose : trace des principales actions (par defaut desactive)\n" ) ; 
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

/*! Gestion des générations : 
 * - toutes les cellules évoluent avec leurs voisins de la génération <code>G</code> pour créer leur état dans la génération <code>G+1</code>
 * - si l'automate possède <code>N</code> cellules, alors on ne régénère l'automate que quand les <code>N</code> cellules ont évoluées
 * - on est dans le même cadre que le TP de L2
 * <br>
 */
int
main( int argc , char * argv[] )
{
  automate_t * automate = NULL ;
  int noerr = CORRECT ;
     
  /*----------*/

  int opt ; 
  char nomprog[512] ;
  booleen_t  verbose = FAUX ;

  /* --- Options --- */
  char * fichier_init = NULL ;
  cellule_regles_t regles = cellule_regles_default() ;
  coords_t * coords_init = coords_new() ; 

  /*! <ol> */

   /*! <li> Captures des options et des paramètres */
  
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
		err_print(noerr) ; exit(0) ; 
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

  /*! <li> Création de l'automate avec les coordonnées des cellules vivantes entrées dans les options -c et -f */

  if( verbose ) printf("Lecture des coordonnees initiales\n");
  if( (noerr = coords_scan( fichier_init , coords_init ) ) )
    {
      fprintf( stderr , "%s : Pb chargement liste des coords initiales a partir de %s\n" , nomprog , ( fichier_init == NULL ? "NULL" : fichier_init ) ) ;
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
  if( (automate = automate_new( hauteur , largeur , coords_init ) ) == NULL )
    {
      fprintf( stderr , "%s : erreur\n" , nomprog ) ;
      exit(1) ; 
    }
  automate_print( stdout , automate ) ;
  
  /* ---------- */
  
  if( verbose ) printf("Evolution synchrone de l'automate\n") ;
  int n = 0 ;
  int i , j ; 
  /*! <li> Pour chaque génération n */
  for( n=1 ; n<nb_generations ; n++ )
    {

      /*!  <ul> */
      /*! <li> Calcul de la prochaine génération pour toutes les cellules n+1 */
      for(i=0;i<hauteur;i++)
	for(j=0;j<largeur;j++)
	  {	
	    if( (noerr = automate_cellule_evoluer( automate , automate_get( automate , i , j ) , &regles ) ) )
	      {
		fprintf( stderr ,"%s : Pb evolution  cellule [%d,%d], sortie erreur %d\n" , nomprog , i, j , noerr ) ;
		err_print(noerr) ; exit(0) ; 
	      }
	  }
      /*! <li> Affichage automate, en tenant compte des générations n et n+1 */
      system("clear") ;
      printf("SYNCHRONE : Generation %d\n" , n+1 ) ; 
      automate_print( stdout , automate ) ;
      usleep(100000);

      /*! <li> Passage à la prochaine génération: pour toutes les cellules la génération n+1 devient la génération n */
      if( (noerr = automate_generer( automate ) ) )
	{
	  fprintf( stderr ,"%s : Pb prise en compte nouvelle generation automate, sortie erreur %d\n" , nomprog , noerr ) ;
	  err_print(noerr) ; exit(0) ; 
	}

      /*!  </ul> */
    }

  /* ---------- */

  /*! <li> Destruction des variables: liste des coordonnées initiales, des cellules et de l'automate */
  
  if( verbose ) printf("Destruction liste de coordonnées d'initialisation\n") ;
  if( ( noerr = coords_destroy( &coords_init ) ) )
    {
      fprintf( stderr ,"%s : Pb destruction des coords d'initialisation, sortie erreur %d\n" , nomprog , noerr ) ;
      err_print(noerr) ; exit(0) ; 
    }
  
  if( verbose ) printf("Destruction automate\n") ; 
  if( (noerr = automate_destroy( &automate ) ) )
    {
      fprintf( stderr , "%s : sortie erreur %d\n" , nomprog , noerr ) ;
      err_print(noerr) ; exit(0) ;  
    }

  /*! </ol> */
  exit(0); 
}
