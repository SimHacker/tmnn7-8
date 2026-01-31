/****************************************************************************

NAME
   locknews -- manipulate permissions on private newsgroups

SYNOPSIS
   locknews

DESCRIPTION
   Type ? to the first % prompt for help, and/or read locknews(1).

FILES
   ADM/authorized	-- what this edits
   ADM/authorized.new	-- temp file used for editing

BUGS
   There's no locking to protect multiple concurrent instances of locknews
from stepping on each other, yet. This will be fixed.
   The p and r commands only modify 'permit' lines, may be rendered
ineffective by 'forbid' lines, and will fail as through the group name
were invalid if only 'forbid' lines for the group are present.

AUTHOR
   Eric S. Raymond
   This software is Copyright (C) 1989 by Eric S. Raymond for the sole purpose
of protecting free redistribution; see the LICENSE file for details.

**************************************************************************/
/*LINTLIBRARY*/
#include "news.h"
#include "fascist.h"

char	*Progname = "newslock";

private char	oldperms[LBUFLEN];	/* name of old authorization file */
private char	newperms[LBUFLEN];	/* name of edited authorization file */
private char	newdata[LBUFLEN];	/* where to buffer edited fields */
private char	allgrps[LBUFLEN];	/* lists all declared group names */
private char	keeper[LBUFLEN];	/* group list caller is keeper for */

/* editor methods begin here */

private int adduser(field, largc, largv)
char	*field[];
int	largc;
char	*largv[];
{
    (void) strlcpy(newdata, field[F_MEMBERS], sizeof(newdata));
    for (; largc--; largv++)
	if (ngmatch(largv[0], field[F_MEMBERS])
	    || ngmatch(largv[0], field[F_KEEPERS]))
	{
	    (void) printf("%s is already in %s\n", largv[0], field[F_GROUP]);
	    return(FAIL);
	}
        else
	{
	    (void) strlcat(newdata, ",", sizeof(newdata));
	    (void) strlcat(newdata, largv[0], sizeof(newdata));
	}
	(void) printf("Group %s now has users %s\n",
		      field[F_GROUP], field[F_MEMBERS] = newdata);
    return(SUCCEED);
}

private int deluser(field, largc, largv)
char	*field[];
int	largc;
char	*largv[];
{
    for (; largc--; largv++)
	if (!ngmatch(largv[0], field[F_MEMBERS]))
	{
	    (void) printf("%s is not in %s\n", largv[0], field[F_GROUP]);
	    return(FAIL);
	}
        else
	{
	    int		namelen = strlen(largv[0]);
	    char	*cp = field[F_MEMBERS], *tp = newdata;

	    while (*cp)
		if (!strncmp(largv[0], cp, namelen)
			&& ((cp[namelen] == ',') || (cp[namelen] == '\0')))
		{
		    cp += namelen;
		    if (*cp == ',')
			cp++;
		    if ((*cp == F_FIELDSEP[0] || *cp == '\0') && tp[-1] == ',')
			tp--;
		}
		else
		    *tp++ = *cp++;
	    *tp = '\0';
	}

    (void) printf("Group %s now has users %s\n",
		  field[F_GROUP], field[F_MEMBERS] = newdata);
    return(SUCCEED);
}

/* editor methods end here */

private void crackline(line, fv)
/* crack a line into F_FIELDSEP[0]-separated fields */
char	*line;
char	*fv[];
{
    int	fld;
    char *cp;

    /* so crack line into fields */
    (void) nstrip(line);
    fv[fld = 0] = strtok(line, F_FIELDSEP);
    while ((cp = strtok((char *)NULL, F_FIELDSEP)) != (char *)NULL)
	if (fld < F_MAXFLDS)
	    fv[++fld] = cp;
    fv[++fld] = "";
}

private int fetchline(cmd, grpname, field)
/* fetch line with group field matching grpname and desired command type */
char	cmd;
char	*grpname;
char	*field[];
{
    FILE	*rfp;
    int		lineno = 0;
    bool	foundit = FALSE;
    char	oldentry[BUFLEN];

    /* first arg must always be a valid group name */
    rfp = xfopen(oldperms, "r");
    while (fgets(oldentry, BUFLEN, rfp) != (char *)NULL)
    {
	crackline(oldentry, field);
	lineno++;

	/* process only lines that match the given group */
	if (strcmp(grpname, field[F_GROUP]) == 0)
	    if ((strchr("adl", cmd) && field[0][0] == 'g')
			|| (strchr("pr", cmd) && field[0][0] == 'p'))
	    {
		foundit = TRUE;
		break;
	    }
     }
    (void) fclose(rfp);

    return(foundit ? lineno : 0);
}
    
private int rewrite(lineno, field)
/* rewrite permissions file to make line lineno have given fields */
int	lineno;
char	*field[];
{
    FILE	*rfp, *wfp;
    int		lc;

    /* O.K, the command modified field[], now write all data back out */
    rfp = xfopen(oldperms, "r");
    wfp = xfopen(newperms, "w");
    lc = 0;

    if (field[0][0] == 'g')
	(void)fprintf(wfp,"%s:%s:%s:%s\n",field[0],field[1],field[2],field[3]);
    while (fgets(bfr, BUFLEN, rfp) != (char *)NULL)
	if (++lc != lineno)
	    (void) fputs(bfr, wfp);
    if (field[0][0] != 'g')
	(void)fprintf(wfp,"%s:%s:%s:%s\n",field[0],field[1],field[2],field[3]);

    (void) fclose(rfp);
    (void) fclose(wfp);

    /* and make the edited form into the authorization file */
    if (rename(newperms, oldperms) == FAIL)
    {
	(void) fprintf(stderr, "couldn't link %s back to %s\n",
		       newperms, oldperms);
	return(FAIL);
    }

    return(SUCCEED);
}

private int edit(largc, largv)
int	largc;
char	*largv[];
{
    int		lineno = 0;
    char	*field[F_MAXFLDS + 1];

    /* verify user's permission to alter the line, crack its fields */
    if (strchr("adprl", largv[F_COMMAND][0]))
    {
	/* first, check that the group name is really valid */
	if (!ngmatch(largv[F_GROUP], allgrps))
	{
	    (void) printf("%s is not a valid group name.\n", largv[1]);
	    return(FAIL);
	}

	/*
	 * Anyone logged in as the news owner can edit any group (remember
	 * that because newsinit() cracks username out of /etc/passwd using
	 * the uid this is hard to fool -- is this going to be untenable in a
	 * distributed environment?). Of course, anyone registered as a
	 * gatekeeper for the group can also edit.
	 */
	if (strcmp(username, NEWSUSR) && !ngmatch(largv[F_GROUP], keeper))
	{
	    (void) printf("You don't have gatekeeper privileges for %s.\n",
			  largv[F_GROUP]);
	    return(FAIL);
	}
    }

    /* now dispatch on the command we were handed */
    switch(largv[0][0])
    {
    case 'a':
	if ((lineno = fetchline(largv[0][0], largv[F_GROUP], field)) == 0)
	{
	    (void) printf("Sorry, locknews can't add a user to %s.\n",
			  largv[F_GROUP]);
	    return(FAIL);
	}
	else if (adduser(field, largc - 2, largv + 2) == SUCCEED)
	    return(rewrite(lineno, field));
	else
	    return(FAIL);

    case 'd':
	if ((lineno = fetchline(largv[0][0], largv[F_GROUP], field)) == 0)
	{
	    (void) printf("Sorry, locknews can't remove a user from %s.\n",
			  largv[F_GROUP]);
	    return(FAIL);
	}
	else if (deluser(field, largc - 2, largv + 2) == SUCCEED)
	    return(rewrite(lineno, field));
	else
	    return(FAIL);

    case 'p':
	if ((lineno = fetchline(largv[0][0], largv[F_GROUP], field)) == 0)
	{
	    field[F_COMMAND] = "permit";
	    field[F_GROUP] = largv[F_GROUP];
	    field[F_READSUBSC] = "";
	    (void) fputs("Creating new entry...", stdout);
	}
	if (largv[F_COMMAND][1] == '+')
	{
	    (void) strlcpy(newdata, field[F_POSTSUBSC], sizeof(newdata));
	    (void) strlcat(newdata, ",", sizeof(newdata));
	}
	else
	    newdata[0] = '\0';
	(void) strlcat(newdata, largv[2], sizeof(newdata));
	(void) printf("New post subscription for %s is: %s\n",
		      field[F_GROUP], field[F_POSTSUBSC] = newdata);
	return(rewrite(lineno, field));

    case 'r':
	if ((lineno = fetchline(largv[F_COMMAND][0],largv[F_GROUP],field)) ==0)
	{
	    field[F_COMMAND] = "permit";
	    field[F_GROUP] = largv[F_GROUP];
	    field[F_POSTSUBSC] = field[F_READSUBSC + 1] = "";
	    (void) fputs("Creating new entry...", stdout);
	}
	if (largv[F_COMMAND][1] == '+')
	{
	    (void) strlcpy(newdata, field[F_READSUBSC], sizeof(newdata));
	    (void) strlcat(newdata, ",", sizeof(newdata));
	}
	else
	    newdata[0] = '\0';
	(void) strlcat(newdata, largv[2], sizeof(newdata));
	(void) printf("New read subscription for %s is: %s\n",
		      field[F_GROUP], field[F_READSUBSC] = newdata);
	return(rewrite(lineno, field));

    case 'l':
	(void) printf("Group: %s\n", field[F_GROUP]);
	(void) printf("Members: %s\n", field[F_MEMBERS]);
	(void) printf("Gatekeepers: %s\n", field[F_KEEPERS]);
	return(SUCCEED);

    default:
	(void) printf("locknews: illegal command\n");
	/* FALLTHROUGH */
    case '?':
	(void) printf("a  group user...    -- add user to group\n");
	(void) printf("d  group user...    -- delete user from group\n");
	(void) printf("p  group postsub    -- set post access\n");
	(void) printf("p+ group postsub    -- add to post access\n");
	(void) printf("r  group readsub    -- set read access\n");
	(void) printf("r+ group readsub    -- add to read access\n");
	(void) printf("l  group            -- print status\n");
	(void) printf("x  group            -- exit\n");
	return(FAIL);
    }
}

main(argc, argv)
/* main sequence of the lock editor */
int	argc;
char	**argv;
{
    int		status, largc;
    char	oldentry[BUFLEN], cmdline[LBUFLEN], *largv[F_MAXFLDS + 2];
    FILE	*rfp;
    struct group	*gr;

    newsinit();

    (void) snprintf(oldperms, sizeof(oldperms), "%s/authorized", site.admdir);

    /*
     * first, compile a list of all groups
     * and check the calling user's group authorizations
     */
    /* start with `internal' netnews-only groups */
    rfp = xfopen(oldperms, "r");
    while (fgets(oldentry, BUFLEN, rfp) != (char *)NULL)
    {
	crackline(oldentry, largv);
	if (largv[F_COMMAND][0] == 'g')
	{
	    /* add this group to the list of valid ones */
	    if (strlen(allgrps)+strlen(largv[F_GROUP])+1 > sizeof(allgrps))
	    {
		(void) fprintf(stderr, "locknews: too many groups\n");
		exit(1);
	    }
	    else
	    {
		(void) strlcat(allgrps, ",", sizeof(allgrps));
		(void) strlcat(allgrps, largv[F_GROUP], sizeof(allgrps));
	    }

	    /* bounds checking via strlcat */
	    if (ngmatch(username, largv[F_KEEPERS]))
	    {
		if (keeper[0] != '\0')
		    (void) strlcat(keeper, ",", sizeof(keeper));
		(void) strlcat(keeper, largv[F_GROUP], sizeof(keeper));
	    }
	}
    }		

    (void) fclose(rfp);

    /* now add the `external' groups defined by /etc/groups or equivalent */
#ifndef lint	/* USG and BSD disagree on the type of setgrent() */
    setgrent();
#endif /* lint */
    while ((gr = getgrent()) != (struct group *)NULL)
    {
	if (strlen(allgrps) + strlen(gr->gr_name) + 1 > sizeof(allgrps))
	{
	    (void) fprintf(stderr, "locknews: too many groups\n");
	    exit(1);
	}
	else
	{
	    if (allgrps[0] == '\0')
		(void) strlcat(allgrps, ",", sizeof(allgrps));
	    (void) strlcat(allgrps, gr->gr_name, sizeof(allgrps));
	}
    }
#ifndef lint	/* USG and BSD disagree on the type of setgrent() */
    endgrent();
#endif /* lint */

    /* non-NEWSUSR users that are not gatekeepers should get lost */
    if (strcmp(username, NEWSUSR) && keeper[0] == '\0')
    {
	(void) printf("locknews: %s has no gatekeeper privileges\n",
		      username);
	exit(1);
    }

    (void) snprintf(newperms, sizeof(newperms), "%s/authorized.tmp", site.admdir);

    if (argc > 1)
	status = edit(argc - 1, argv + 1);
    else
    {
	(void) printf("This is the permissions editor, type ? for help\n");
	if (strcmp(username, NEWSUSR))
	    (void) printf("You have gatekeeper privileges for: %s\n", keeper);

	for (;;)
	{
	    if (fputs("% ", stdout) == EOF || fgets(cmdline, sizeof(cmdline), stdin) == (char *)NULL)
		break;

	    /* first, check for exit commands */
	    if (cmdline[0] == 'x' || cmdline[0] == 'q')
		break;

	    /* otherwise, parse for an edit command */
	    largc = vcrack(cmdline, largv, F_MAXFLDS);

	    /* and execute it */
	    (void) edit(largc, largv);
	}
    }
    exit(status);
    /*NOTREACHED*/
}

catch_t xxit(status) int status;	{exit(status);}

/* locknews.c ends here */
