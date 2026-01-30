/*
	testdir -- basic test for C library directory access routines

	last edit:	25-Apr-1987	D A Gwyn
*/

#include	<sys/types.h>
#include	<stdio.h>
#include	<dirent.h>

extern void	exit();
extern int	strcmp();

main( argc, argv )
	int			argc;
	register char		**argv;
	{
	register DIR		*dirp;
	register struct dirent	*dp;
	int			nerrs = 0;	/* total not found */

	if ( (dirp = opendir( "." )) == NULL )
		{
		(void)fprintf( stderr, "Cannot open \".\" directory\n" );
		exit( 1 );
		}

	while ( --argc > 0 )
		{
		++argv;

		while ( (dp = readdir( dirp )) != NULL )
			if ( strcmp( dp->d_name, *argv ) == 0 )
				{
				(void)printf( "\"%s\" found.\n", *argv );
				break;
				}

		if ( dp == NULL )
			{
			(void)printf( "\"%s\" not found.\n", *argv );
			++nerrs;
			}

		rewinddir( dirp );
		}

	(void)closedir( dirp );
	exit( nerrs );
	}
