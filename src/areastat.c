/* $Id$ */

/***************************************************************************
 *                                                                         *
 *  AreaStat Source, Version 1.4.0-stable                                  *
 *  Copyleft (c) 2004 by The Husky project                                *
 *  Homepage: http://husky.sourceforge.net                                 *
 *                                                                         *
 *  EchoStat Source, Version 1.06                                          *
 *  Copyright (c) 2000 by Dmitry Rusov. (2:5090/94, rusov94@mail.ru)       *
 *  All rights reserved.                                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <huskylib/huskylib.h>
//#include <smapi/typedefs.h>
//#include <smapi/progprot.h>
#include <smapi/msgapi.h>

#include "areastat.h"
#include "version.h"

#define BUFSIZE 4096
#define nfree(a) { if (a != NULL) { free(a); a = NULL; } }
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISLWSP(ch) ((ch) == ' ' || (ch) == '\t')

#define secs_in_day 86400

typedef struct area {
    char    *name;
    char    *path;
    char    *out_file;
    int     type;
} s_area, *ps_area;

typedef struct config {

    unsigned long by_name;
    unsigned long by_from;
    unsigned long by_to;
    unsigned long by_from_to;
    unsigned long by_total;
    unsigned long by_size;
    unsigned long by_qpercent;
    unsigned long by_subj;
    unsigned long by_date;
    unsigned long by_wday;
    unsigned long by_time;
    unsigned long days_of_stat;
    unsigned long areas_count;
    unsigned long pkt_size;
    unsigned char make_pkt;
    hs_addr pkt_orig_addr;
    hs_addr pkt_dest_addr;
    char pkt_password[8];
    char *pkt_from;
    char *pkt_to;
    char *pkt_subj;
    char *pkt_origin;
    char *pkt_inbound;
    ps_area  areas;

} s_config, *ps_config;

ps_config main_config;

typedef struct unsorted_item {
    char *name;
    unsigned long from;
    unsigned long to;
} s_unsorted_item, *ps_unsorted_item;

typedef struct named_item {
    char *name;
    unsigned long count;
} s_named_item, *ps_named_item;

typedef struct date_item {
    time_t date;
    struct tm tm_date;
    unsigned long count;
} s_date_item, *ps_date_item;

typedef struct time_item {
    unsigned char hour;
    unsigned long count;
} s_time_item, *ps_time_item;

typedef struct size_item {
    char *name;
    unsigned long size;
    unsigned long qsize;
    float qpercent;
} s_size_item, *ps_size_item;

ps_unsorted_item  global_data = NULL;
unsigned long gd_count = 0;

ps_named_item from_data = NULL;
unsigned long fd_count = 0;

ps_named_item to_data = NULL;
unsigned long td_count = 0;

ps_named_item from_to_data = NULL;
unsigned long ftd_count = 0;

ps_named_item total_data = NULL;
unsigned long ttd_count = 0;

ps_named_item subj_data = NULL;
unsigned long sd_count = 0;

ps_date_item date_data = NULL;
unsigned long dd_count = 0;

ps_time_item time_data = NULL;
unsigned long tttd_count = 0;

ps_size_item size_data = NULL;
unsigned long szd_count = 0;

time_t global_msgid;
unsigned long days = 0;
unsigned long messages = 0;
float msgs_in_day = 0;
FILE * current_std;
FILE *out_pkt;
char *versionStr;
unsigned int isecs;

static int aDaysFromJan1st[13] = {
    0,                                  // Jan
    31,                                 // Feb
    31+28,                              // Mar
    31+28+31,                           // Apr
    31+28+31+30,                        // May
    31+28+31+30+31,                     // Jun
    31+28+31+30+31+30,                  // Jul
    31+28+31+30+31+30+31,               // Aug
    31+28+31+30+31+30+31+31,            // Sep
    31+28+31+30+31+30+31+31+30,         // Oct
    31+28+31+30+31+30+31+31+30+31,      // Nov
    31+28+31+30+31+30+31+31+30+31+30,   // Dec
    31+28+31+30+31+30+31+31+30+31+30+31 // Jan
};

static int days_in_months[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

#ifndef _MAKE_DLL

void *smalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        fprintf(stderr, "\n\nOut of memory\n\n");
        exit(2);
    }
    return ptr;
}

void *srealloc(void *ptr, size_t size)
{
    void *newptr = realloc(ptr, size);
    if (newptr == NULL)
    {
        fprintf(stderr, "\n\nOut of memory\n\n");
        exit(2);
    }
    return newptr;
}

#endif

int name_comparer(const void *str1, const void *str2)
{
    return strcmp(((ps_unsorted_item)str1)->name,((ps_unsorted_item)str2)->name);
}

int ft_comparer(const void *str1, const void *str2)
{
    return strcmp(((ps_named_item)str1)->name,((ps_named_item)str2)->name);
}

int ftc_comparer(const void *count1, const void *count2)
{
    if (((ps_named_item) count1) -> count > ((ps_named_item) count2) -> count) return -1;
    if (((ps_named_item) count1) -> count == ((ps_named_item) count2) -> count) return 0;
    return 1;
}

int size_comparer(const void *size1, const void *size2)
{
    if (((ps_size_item) size1) -> size > ((ps_size_item) size2) -> size) return -1;
    if (((ps_size_item) size1) -> size == ((ps_size_item) size2) -> size) return 0;
    return 1;
}

int qp_comparer(const void *qp1, const void *qp2)
{
    if (((ps_size_item) qp1) -> qpercent > ((ps_size_item) qp2) -> qpercent) return -1;
    if (((ps_size_item) qp1) -> qpercent == ((ps_size_item) qp2) -> qpercent) return 0;
    return 1;
}

int date_comparer(const void *dt1, const void *dt2)
{
    if (((ps_date_item) dt1) -> date < ((ps_date_item) dt2) -> date) return -1;
    if (((ps_date_item) dt1) -> date == ((ps_date_item) dt2) -> date) return 0;
    return 1;
}

void do_sort_gap(unsigned long count, ps_named_item arr)
{
    unsigned long i=0,j=0;
    while (j < count)
    {
        while (i < count)
        {
            i++;
            if (i >= count && arr[i-1].count == arr[j].count)
            {
                qsort (&arr[j], i-j, sizeof(s_named_item), ft_comparer);
                break;
            }
            if (arr[i].count != arr[j].count)
            {
                qsort (&arr[j], i-j, sizeof(s_named_item), ft_comparer);
                break;
            }
        }
        j = i;
    }
}

/* Next 4 routines was taken from Pete Kvitek's FMA */

int DoHasAddrChar(char * pch, char ch)
{
    while (*pch && !ISLWSP(*pch))
        if (*pch == ch) return 1;
        else pch++;
    return 0;
}

int DoIsAddrChar(char ch)
{
    return (ch && strchr("0123456789:/.", ch));
}

char * ScanNetAddr(hs_addr * pnetAddr, char * psz)
{
    char * pch, * pchEnd;

    for (pch = psz; ISLWSP(*pch); pch++);
    pchEnd = pch;

    if (DoIsAddrChar(*pch) && DoHasAddrChar(pch, ':'))
    {
        while (ISDIGIT(*pchEnd)) pchEnd++;
        if (*pchEnd != ':' || pch == pchEnd) return NULL;
        pnetAddr->zone = (unsigned short)atoi(pch);
        pch = ++pchEnd;
        if (!ISDIGIT(*pch) || !DoHasAddrChar(pch, '/')) return NULL;
    }

    if (DoIsAddrChar(*pch) && DoHasAddrChar(pch, '/'))
    {
        while (ISDIGIT(*pchEnd)) pchEnd++;
        if (*pchEnd != '/' || pch == pchEnd) return NULL;
        pnetAddr->net = (unsigned short)atoi(pch);
        pch = ++pchEnd;
        if (!ISDIGIT(*pch)) return NULL;
    }

    if (DoIsAddrChar(*pch) && *pch != '.')
    {
        while (ISDIGIT(*pchEnd)) pchEnd++;
        pnetAddr->node = (unsigned short)atoi(pch);
        pch = pchEnd;
    }

    if (*pch != '.') {
        pnetAddr->point = 0;
    } else {
        for (pchEnd = ++pch; ISDIGIT(*pchEnd); pchEnd++);
        pnetAddr->point = (unsigned short)atoi(pch);
        pch = pchEnd;
    }

    return (pnetAddr->zone && pnetAddr->net) ? pch : NULL;

}

unsigned long get_time_from_stamp (struct _stamp mtime)
{
    unsigned long hour, min, sec;
    unsigned long day, mon, year;
    unsigned long nDaysSince1970;
    unsigned long nYearsSince1970;

    hour = mtime.time.hh + 16;
    min  = mtime.time.mm;
    sec  = mtime.time.ss * 2;
    day  = mtime.date.da;
    mon  = mtime.date.mo;
    year = mtime.date.yr + 80;        // Years since 1900

    nYearsSince1970 = year - 70;

    nDaysSince1970 = nYearsSince1970 * 365 + ((nYearsSince1970 + 1) / 4);

    nDaysSince1970 += aDaysFromJan1st[mon - 1] + (day - 1);

    if ((year & 3) == 0 && mon > 2) nDaysSince1970++;

    return (((unsigned long) nDaysSince1970 * (24L * 60L * 60L))
            + ((unsigned long) hour * (60L * 60L))
            + ((unsigned long) min * 60L)
            +  (unsigned long) sec);

}

int reading_base(unsigned long ii)
{
    XMSG msg;
    HAREA in_area;
    HMSG in_msg;
    void *ptr;
    char ctrl[1], buffer[BUFSIZE+1], *dummy, *c;
    dword offset, msgn, qsize, textlen;
    long got,i;
    long t1, nf, nt, qpos;
    struct _minf mi;
    time_t atime = time(NULL);
    time_t mtime = 0, period = 0;
    struct tm tmp_tm;

    gd_count = fd_count = td_count = ftd_count = ttd_count =
        sd_count = dd_count = szd_count = tttd_count = 0;
    days = 0; messages = 0; msgs_in_day = 0;


    fprintf(stderr,"Scanning area %s, %s...\n",
            main_config->areas[ii].name,main_config->areas[ii].path);

    if (main_config->areas[ii].type == 1) t1 = MSGTYPE_SDM; else
        if (main_config->areas[ii].type == 2) t1 = MSGTYPE_SQUISH; else
            if (main_config->areas[ii].type == 3) t1 = MSGTYPE_JAM;

    memset(&mi, '\0', sizeof mi);
    mi.def_zone=2;

    MsgOpenApi(&mi);

    if ((in_area=MsgOpenArea(main_config->areas[ii].path, MSGAREA_NORMAL, t1))==NULL)
    {
        fprintf(stderr,"Error opening area `%s' (type %d) for read!\n\n",
                main_config->areas[ii].path, t1);
        exit(1);
    }

    MsgLock(in_area);

    for (msgn=1L; msgn <= MsgHighMsg(in_area); msgn++)
    {
        if ((msgn % 5)==0)
        {
            fprintf(stderr,"Scanning msg: %ld\r",msgn);
            fflush(stdout);
        }
        if ((in_msg=MsgOpenMsg(in_area,MOPEN_READ,msgn))==NULL) continue;
        ptr = in_msg;
        MsgReadMsg(in_msg, &msg, 0L, 0L, NULL, 0, ctrl);
        qsize = 0;
        textlen = MsgGetTextLen(in_msg);
        for (offset=0L; offset < textlen;)
        {
            if ((textlen-offset) >= BUFSIZE)
                got = MsgReadMsg(in_msg, NULL, offset, BUFSIZE, buffer, 0L, NULL);
            else
                got = MsgReadMsg(in_msg, NULL, offset, textlen-offset, buffer, 0L, NULL);

            if (got == 0)
                break; /* we read 0 bytes - the end of message */
            offset += got;
            // find a quote
            c = strtok (buffer, "\n\r");
            while (c != NULL)
            {
                c = strtok (NULL,"\n\r");
                if (c != NULL)
                {
                    for (qpos = 0; qpos<10 && *c; qpos++)
                    {
                        if (*c == '>')
                        {
                            qsize += strlen (c);
                            break;
                        }
                        c++;
                    }
                }
            }
            if (got <= 0)
                break;
        } /* for */


/*        printf("qsize=%d\n",qsize); */

        in_msg = ptr;
        MsgCloseMsg(in_msg);

        mtime = get_time_from_stamp(msg.date_written);

#ifndef __MVC__

        mtime += 43200;

#endif
        period = atime - mtime;
        tmp_tm = *localtime (&mtime);
        tmp_tm.tm_mon++;
        //printf("%02d-%02d-%04d,",tmp_tm.tm_mday,tmp_tm.tm_mon,tmp_tm.tm_year+1900);
        //printf("%02d:%02d:%02d.",tmp_tm.tm_hour,tmp_tm.tm_hour,tmp_tm.tm_min,tmp_tm.tm_sec);
#if 0
        printf("\nmessage time: %s",ctime(&mtime));
        printf("message time: %02d-%02d-%04d  %02d:%02d:%02d\n",tmp_tm.tm_mday,
               tmp_tm.tm_mon,tmp_tm.tm_year+1900,tmp_tm.tm_hour,
               tmp_tm.tm_min,tmp_tm.tm_sec);
#endif
        if (!period || period > main_config->days_of_stat * secs_in_day) continue;
        messages++;
        // by name
        nf = 0; nt = 0;
        for (i=0; i<gd_count; i++)
        {
            if (stricmp(global_data[i].name,msg.from) == 0)
            {
                global_data[i].from++;
                nf = 1;
            }
            if (stricmp(global_data[i].name,msg.to) == 0)
            {
                global_data[i].to++;
                nt = 1;
            }
        }

        if (!nf)
        {
            gd_count++;
            global_data = srealloc (global_data, gd_count * sizeof(s_unsorted_item));
            global_data[gd_count-1].name = smalloc(strlen(msg.from)+1);
            strcpy (global_data[gd_count-1].name,msg.from);
            global_data[gd_count-1].from = 1;
            global_data[gd_count-1].to = 0;
        }


        if (!nt)
        {
            gd_count++;
            global_data = srealloc (global_data, gd_count * sizeof(s_unsorted_item));
            global_data[gd_count-1].name = smalloc(strlen(msg.to)+1);
            strcpy (global_data[gd_count-1].name,msg.to);
            global_data[gd_count-1].from = 0;
            global_data[gd_count-1].to = 1;
        }
        // by from
        if (main_config->by_from)
        {
            nf = 0;
            for (i = 0; i < fd_count; i++)
            {
                if (!stricmp(from_data[i].name,msg.from))
                {
                    from_data[i].count++;
                    nf = 1;
                }
            }

            if (!nf)
            {
                fd_count++;
                from_data = srealloc (from_data,fd_count * sizeof(s_named_item));
                from_data[fd_count-1].name = (char *) smalloc(strlen(msg.from)+1);
                strcpy (from_data[fd_count-1].name,msg.from);
                from_data[fd_count-1].count = 1;
            }
        }
        // by to
        if (main_config->by_to)
        {
            nf = 0;
            for (i = 0; i < td_count; i++)
            {
                if (!stricmp(to_data[i].name,msg.to))
                {
                    to_data[i].count++;
                    nf = 1;
                }
            }

            if (!nf)
            {
                td_count++;
                to_data = srealloc (to_data, td_count * sizeof(s_named_item));
                to_data[td_count-1].name = (char *) smalloc(strlen(msg.to)+1);
                strcpy (to_data[td_count-1].name,msg.to);
                to_data[td_count-1].count = 1;
            }
        }
        // by subj
        if (main_config->by_subj)
        {
            nf = 0;
            for (i = 0; i < sd_count; i++)
            {
                if (!stricmp(subj_data[i].name,msg.subj))
                {
                    subj_data[i].count++;
                    nf = 1;
                }
            }

            if (!nf)
            {
                sd_count++;
                subj_data = srealloc (subj_data, sd_count * sizeof(s_named_item));
                subj_data[sd_count-1].name = (char *) smalloc(strlen(msg.subj)+1);
                strcpy (subj_data[sd_count-1].name,msg.subj);
                subj_data[sd_count-1].count = 1;
            }
        }

        // by date
        nf = 0;
        for (i = 0; i < dd_count; i++)
        {
            if (tmp_tm.tm_mday == date_data[i].tm_date.tm_mday &&
                tmp_tm.tm_mon == date_data[i].tm_date.tm_mon &&
                tmp_tm.tm_year == date_data[i].tm_date.tm_year)
            {
                date_data[i].count++;
                nf = 1;
            }
        }

        if (!nf)
        {
            dd_count++;
            date_data = srealloc (date_data, dd_count * sizeof(s_date_item));
            date_data[dd_count-1].date = mtime;
            date_data[dd_count-1].tm_date = tmp_tm;
            date_data[dd_count-1].count = 1;
        }

        // by time
        if (main_config->by_time)
        {
            nf = 0;
            for (i = 0; i < tttd_count; i++)
            {
                if (tmp_tm.tm_hour == time_data[i].hour)
                {
                    time_data[i].count++;
                    nf = 1;
                }
            }

            if (!nf)
            {
                tttd_count++;
                time_data = srealloc (time_data, tttd_count * sizeof(s_time_item));
                time_data[tttd_count-1].hour = tmp_tm.tm_hour;
                time_data[tttd_count-1].count = 1;
            }
        }

        // by size
        if (main_config->by_size)
        {
            nf = 0;
            for (i = 0; i < szd_count; i++)
            {
                if (!stricmp(size_data[i].name,msg.from))
                {
                    size_data[i].size += offset;
                    size_data[i].qsize += qsize;
                    nf = 1;
                }
            }

            if (!nf)
            {
                szd_count++;
                size_data = srealloc (size_data, szd_count * sizeof(s_size_item));
                size_data[szd_count-1].name = (char *) smalloc(strlen(msg.from)+1);
                strcpy (size_data[szd_count-1].name,msg.from);
                size_data[szd_count-1].size = offset;
                size_data[szd_count-1].qsize = qsize;
            }
        }

        // by from -> to
        if (main_config->by_from_to)
        {

            nf = 0;
            dummy = (char *) smalloc (strlen(msg.from)+strlen(msg.to)+2);
            strcpy (dummy,msg.from);
            strcat (dummy,"");
            strcat (dummy,msg.to);
            for (i = 0; i < ftd_count; i++)
            {
                if (!stricmp(from_to_data[i].name,dummy))
                {
                    from_to_data[i].count++;
                    nf = 1;
                    free(dummy);
                }
            }

            if (!nf)
            {
                ftd_count++;
                from_to_data = srealloc (from_to_data, ftd_count * sizeof(s_named_item));
                from_to_data[ftd_count-1].name = dummy;
                from_to_data[ftd_count-1].count = 1;
            }
        }

    } /* for msgn */

    qsort (date_data, dd_count, sizeof(s_date_item), date_comparer);

    MsgCloseArea(in_area);
    MsgCloseApi();

    return 0;
}

int free_all()
{
    int i;

    for (i = 0; i<gd_count; i++)
    {
        free(global_data[i].name);
    }

    if (global_data) free (global_data);
    return 0;
}

int print_summary_statistics(unsigned long i)
{
    struct tm fd, ld;
    time_t tt = time(NULL);

    if (dd_count > 0)
    {
        fd = date_data[0].tm_date;
        ld = date_data[dd_count-1].tm_date;
        days = ((date_data[dd_count-1].date-date_data[0].date)/secs_in_day)+1;

    } else  {

        fd = *localtime(&tt);
        ld = *localtime(&tt);
    }

    if (!days) days = 1;
    fprintf(current_std,"\n --- Summary\n\n");

    fprintf(current_std,"  Area: %s\n",main_config->areas[i].name);
    fprintf(current_std,"  Period: %s %02d %s %d - %s %02d %s %d\n",
            weekday_ab[fd.tm_wday],fd.tm_mday,months_ab[fd.tm_mon-1],fd.tm_year+1900,
            weekday_ab[ld.tm_wday],ld.tm_mday,months_ab[ld.tm_mon-1],ld.tm_year+1900);

    fprintf(current_std,"  Messages: %ld\n",messages);
    fprintf(current_std,"  Total users: %ld\n",gd_count);
    fprintf(current_std,"  Active users: %ld\n",fd_count);
    fprintf(current_std,"  Days: %ld\n",days);
    fprintf(current_std,"  Msgs per day: %ld\n",(unsigned long)(float)messages/days);

    return 0;
}

int print_statistics_by_name()
{
    unsigned long i,tot_from = 0, tot_to = 0, tot_tot = 0;

    qsort (global_data, gd_count, sizeof(s_unsorted_item), name_comparer);

    fprintf(current_std,"\n --- Sorted by Name\n");
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Name                                     ³ From ³ To   ³ Total ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´\n");

    for (i=0; i<gd_count; i++)
    {
        fprintf(current_std,"³%4ld.³ %-41s³%6ld³%6ld³%7ld³\n",i+1,global_data[i].name,
                global_data[i].from,global_data[i].to,global_data[i].from+global_data[i].to);
        tot_from += global_data[i].from;
        tot_to += global_data[i].to;

    } /* for */

    tot_tot = tot_from+tot_to;
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄ´\n");
    fprintf(current_std,"³     ³                                          ³%6ld³%6ld³%7ld³\n",tot_from,tot_to,tot_tot);
    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_from()
{
    unsigned long i,j,k;
    char diag[80],name[32];

    qsort (from_data, fd_count, sizeof(s_named_item), ftc_comparer);
    do_sort_gap(fd_count,from_data);

    fprintf(current_std,"\n --- Sorted by From. Top %ld\n",main_config->by_from);

    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Name                           ³ From ³                              ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

    for (i=0; i < fd_count; i++)
    {

        if (i >= main_config->by_from) break;

        strcpy(diag,"\0");
        k = ((float)(from_data[i].count/(float)(from_data[0].count))*30);
        if (k <= 0) strcpy(diag,"Ú"); else
            for (j = 0;j < k ;j++) strcat(diag,"Ü");
        for (j=0; j<32; j++) name[j] = '\0';
        strncpy(name,from_data[i].name,30);

        fprintf(current_std,"³%4ld.³ %-31s³%6ld³%-30s³\n",i+1,name,from_data[i].count,diag);

    } /* for */

    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_to()
{
    unsigned long i,j,k;
    char diag[80],name[32];

    qsort (to_data, td_count, sizeof(s_named_item), ftc_comparer);
    do_sort_gap(td_count,to_data);

    fprintf(current_std,"\n --- Sorted by To. Top %ld\n",main_config->by_to);
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Name                           ³  To  ³                              ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

    for (i=0; i < td_count; i++)
    {

        if (i >= main_config->by_to) break;

        strcpy(diag,"\0");
        k = ((float)(to_data[i].count/(float)(to_data[0].count))*30);
        if (k <= 0) strcpy(diag,"Ú"); else
            for (j = 0;j < k ;j++) strcat(diag,"Ü");
        for (j=0; j<32; j++) name[j] = '\0';
        strncpy(name,to_data[i].name,30);

        fprintf(current_std,"³%4ld.³ %-31s³%6ld³%-30s³\n",i+1,name,to_data[i].count,diag);

    } /* for */

    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_from_to()
{
    unsigned long i,j,k,l,n;
    char diag[80],name1[32],name2[32];

    qsort (from_to_data, ftd_count, sizeof(s_named_item), ftc_comparer);
    do_sort_gap(ftd_count,from_to_data);

    fprintf(current_std,"\n --- Sorted by From -> To. Top %ld\n",main_config->by_from_to);
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ From                 ³ To                    ³ Msgs ³                ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

    for (i=0; i < ftd_count; i++)
    {

        if (i >= main_config->by_from_to) break;

        strcpy(diag,"\0");
        k = ((float)(from_to_data[i].count/(float)(from_to_data[0].count))*16);
        if (k <= 0) strcpy(diag,"Ú"); else
            for (j = 0;j < k ;j++) strcat(diag,"Ü");

        n = 0;

        for (l=0; l<32; l++) name1[l] = '\0';
        for (l=0; l<20; l++)
        {
            if (from_to_data[i].name[n] == '\0' || from_to_data[i].name[n] == '') break;
            name1[l] = from_to_data[i].name[n];
            n++;
        }

        name1[l] = '\0';

        while(from_to_data[i].name[n] && from_to_data[i].name[n] != '') n++;

        for (l=0; l<32; l++) name2[l] = '\0';
        for (l=0; l<20; l++)
        {
            if (from_to_data[i].name[n] == '') n++;
            if (!from_to_data[i].name[n]) break;
            name2[l] = from_to_data[i].name[n];
            n++;
        }

        name2[l] = '\0';

        fprintf(current_std,"³%4ld.³ %-21s³ %-22s³%6ld³%-16s³\n",i+1,name1,name2,from_to_data[i].count,diag);

    } /* for */

    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_total()
{
    unsigned long i,j,k;
    char diag[80],name[32];

    for (i = 0; i < gd_count; i++)
    {
        ttd_count++;
        total_data = srealloc (total_data, ttd_count * sizeof(s_named_item));
        total_data[ttd_count-1].name = (char *) smalloc(strlen(global_data[i].name)+1);
        strcpy (total_data[ttd_count-1].name,global_data[i].name);
        total_data[ttd_count-1].count = global_data[i].from+global_data[i].to;
    }

    qsort (total_data, ttd_count, sizeof(s_named_item), ftc_comparer);
    do_sort_gap(ttd_count,total_data);

    fprintf(current_std,"\n --- Sorted by Total. Top %ld\n",main_config->by_total);
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Name                           ³ Msgs ³                              ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

    for (i=0; i < ttd_count; i++)
    {

        if (i >= main_config->by_total) break;

        strcpy(diag,"\0");
        k = ((float)(total_data[i].count/(float)(total_data[0].count))*30);
        if (k <= 0) strcpy(diag,"Ú"); else
            for (j = 0;j < k ;j++) strcat(diag,"Ü");
        for (j=0; j<32; j++) name[j] = '\0';
        strncpy(name,total_data[i].name,30);

        fprintf(current_std,"³%4ld.³ %-31s³%6ld³%-30s³\n",i+1,name,total_data[i].count,diag);

    } /* for */

    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_size()
{
    unsigned long i,j,k;
    char diag[80],name[32];

    qsort (size_data, szd_count, sizeof(s_size_item), size_comparer);

    fprintf(current_std,"\n --- Sorted by Size. Top %ld\n",main_config->by_size);
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Name                     ³ Size  ³ Quote ³                           ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

    for (i=0; i < szd_count; i++)
    {

        if (i >= main_config->by_size) break;

        strcpy(diag,"\0");
        k = ((float)(size_data[i].size/(float)(size_data[0].size))*27);
        if (k <= 0) strcpy(diag,"Ú"); else
            for (j = 0;j < k ;j++) strcat(diag,"Ü");
        for (j=0; j<32; j++) name[j] = '\0';
        strncpy(name,size_data[i].name,24);

        fprintf(current_std,"³%4ld.³ %-25s³%7ld³%7ld³%-27s³\n",i+1,name,size_data[i].size,size_data[i].qsize,diag);

    } /* for */


    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_qpercent()
{
    unsigned long i,j,k;
    char diag[80],name[32];

    for (i = 0; i < szd_count; i++)
        size_data[i].qpercent = ((float)size_data[i].qsize/(float)size_data[i].size)*100;
    qsort (size_data, szd_count, sizeof(s_size_item), qp_comparer);

    fprintf(current_std,"\n --- Sorted by QPercent. Top %ld\n",main_config->by_qpercent);
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Name                           ³ QPer ³                              ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´\n");

    for (i=0; i < szd_count; i++)
    {

        if (i >= main_config->by_qpercent) break;

        strcpy(diag,"\0");
        if (size_data[0].qpercent >0 )
            k = ((float)(size_data[i].qpercent/(float)(size_data[0].qpercent))*30);
        else k = 0;
        for (j = 0;j < k ;j++) strcat(diag,"Ü");
        for (j=0; j<32; j++) name[j] = '\0';
        strncpy(name,size_data[i].name,30);

        fprintf(current_std,"³%4ld.³ %-31s³%6.2lf³%-30s³\n",i+1,name,size_data[i].qpercent,diag);

    } /* for */

    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_subj()
{
    unsigned long i,j,l;
    char name[60];

    qsort (subj_data, sd_count, sizeof(s_named_item), ftc_comparer);
    do_sort_gap(sd_count,subj_data);

    fprintf(current_std,"\n --- Sorted by Subj. Top %ld\n",main_config->by_subj);
    fprintf(current_std,"ÚÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄ¿\n");
    fprintf(current_std,"³ No. ³ Subj                                                    ³ Msgs ³\n");
    fprintf(current_std,"ÃÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄ´\n");

    for (i=0; i < sd_count; i++)
    {

        if (i >= main_config->by_subj) break;

        l = strlen(subj_data[i].name);

        for (j = 0; j < 56; j++) name[j] = '\0';
        for (j = 0; j < 55 && j < l; j++) name[j] = subj_data[i].name[j];

        fprintf(current_std,"³%4ld.³ %-56s³%6ld³\n",i+1,name,subj_data[i].count);

    } /* for */

    fprintf(current_std,"ÀÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÙ\n");

    return 0;
}

int print_statistics_by_date()
{
    unsigned long i,j,k,maxcount;
    char diag[80];

    fprintf(current_std,"\n --- Sorted by date\n\n");

    maxcount = 0;
    for (i = 0; i < dd_count; i++) if (date_data[i].count > maxcount)
        maxcount = date_data[i].count;

    for (i = 0; i < dd_count; i++)
    {
        strcpy(diag,"\0");
        k = ((float)(date_data[i].count/(float)(maxcount))*59);
        if (k <= 0) strcpy(diag,"Ú"); else
            for (j = 0;j < k ;j++) strcat(diag,"Ü");

        fprintf(current_std," %02d-%02d-%04d %4ld ³%-59s\n",date_data[i].tm_date.tm_mday,
                date_data[i].tm_date.tm_mon,date_data[i].tm_date.tm_year+1900,date_data[i].count,diag);

    } /* for */

    fprintf(current_std,"                 ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ>\n");

    return 0;
}

int print_statistics_by_wdays()
{
    unsigned long i,j,k,maxcount;
    unsigned long counts[7] = {0,0,0,0,0,0,0};
    char diag[80];

    fprintf(current_std,"\n --- Sorted by WeekDay\n\n");

    for (i = 0; i < dd_count; i++)
    {
        switch (date_data[i].tm_date.tm_wday)
        {
        case 0: {counts[0] += date_data[i].count; break;}
        case 1: {counts[1] += date_data[i].count; break;}
        case 2: {counts[2] += date_data[i].count; break;}
        case 3: {counts[3] += date_data[i].count; break;}
        case 4: {counts[4] += date_data[i].count; break;}
        case 5: {counts[5] += date_data[i].count; break;}
        case 6: {counts[6] += date_data[i].count; break;}
        }
    }

    maxcount = 0;
    for (i = 0; i <= 6; i++) if (counts[i] > maxcount) maxcount = counts[i];

    for (i = 0; i <= 6; i++)
    {
        strcpy(diag,"\0");
        k = ((float)(counts[i]/(float)(maxcount))*60);
        for (j = 0;j < k ;j++) strcat(diag,"Ü");

        fprintf(current_std," %s %6ld³%-60s\n",weekday_ab[i],counts[i],diag);

    }

    fprintf(current_std,"           ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ>\n");
    return 0;
}

int print_statistics_by_time()
{
    unsigned long i,j,k,maxcount;
    unsigned long counts[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    char diag[80], str1[] = "ú  ", str2[] = "   ";

    fprintf(current_std,"\n --- Sorted by time\n\n");

    for (i = 0; i < tttd_count; i++)
    {
        switch (time_data[i].hour)
        {
        case 0: {counts[0] += time_data[i].count; break;}
        case 1: {counts[1] += time_data[i].count; break;}
        case 2: {counts[2] += time_data[i].count; break;}
        case 3: {counts[3] += time_data[i].count; break;}
        case 4: {counts[4] += time_data[i].count; break;}
        case 5: {counts[5] += time_data[i].count; break;}
        case 6: {counts[6] += time_data[i].count; break;}
        case 7: {counts[7] += time_data[i].count; break;}
        case 8: {counts[8] += time_data[i].count; break;}
        case 9: {counts[9] += time_data[i].count; break;}
        case 10: {counts[10] += time_data[i].count; break;}
        case 11: {counts[11] += time_data[i].count; break;}
        case 12: {counts[12] += time_data[i].count; break;}
        case 13: {counts[13] += time_data[i].count; break;}
        case 14: {counts[14] += time_data[i].count; break;}
        case 15: {counts[15] += time_data[i].count; break;}
        case 16: {counts[16] += time_data[i].count; break;}
        case 17: {counts[17] += time_data[i].count; break;}
        case 18: {counts[18] += time_data[i].count; break;}
        case 19: {counts[19] += time_data[i].count; break;}
        case 20: {counts[20] += time_data[i].count; break;}
        case 21: {counts[21] += time_data[i].count; break;}
        case 22: {counts[22] += time_data[i].count; break;}
        case 23: {counts[23] += time_data[i].count; break;}
        }
    }

    maxcount = 0;
    for (i = 0; i <= 23; i++) if (counts[i] > maxcount) maxcount = counts[i];

    for (j = 20; j > 0; j--)
    {

        strcpy(diag,"\0");

        for (i = 0; i <= 23; i++)
        {
            k = ((float)(counts[i]/(float)(maxcount))*20);

            if (k >= j) strcat(diag,"ŞÛ "); else

                if ((j % 5) == 0) strcat (diag,str1); else  strcat (diag,str2);

        }

        if (j == 20)  fprintf(current_std,"100%%Â%s\n",diag); else
            if (j == 15)  fprintf(current_std," 75%%Å%s\n",diag); else
                if (j == 10)  fprintf(current_std," 50%%Å%s\n",diag); else
                    if (j == 5)  fprintf(current_std," 25%%Å%s\n",diag); else
                        fprintf(current_std,"    ³%s\n",diag);

    }

    fprintf(current_std,"  0%%ÀÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄÄÂÄ>\n");
    fprintf(current_std,"      0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 \n");

    return 0;
}

void print_err(char *s, unsigned long i)
{
    printf("Error in config (line %ld): \"%s\"\n\n",i,s);
    exit(2);
}

s_config *read_cfg(char *cfg_name)
{

    FILE        *fcfg;
    s_config    *config;
    char        *s0,*s1,*s2,*s3,*s4,*buff;
    char        line[BUFSIZE+1];
    time_t      at = time (NULL);
    struct tm   t;
    unsigned long i = 0, j = 0, n, wd, md;

    if ((fcfg = fopen(cfg_name,"rb")) == NULL)
    {
        fprintf(stderr,"Cannot open file: \"%s\"\n\n",cfg_name);
        exit(2);
    }

    t = *localtime (&at);

    wd = t.tm_wday; if (wd == 0) wd = 7;

    config = (s_config *) smalloc(sizeof(s_config));

    memset(config, 0, sizeof(s_config));

    config->by_from = config->by_to = config->by_from_to = config->by_total =
        config->by_size = config->by_qpercent = config->by_subj = config->by_name =
        config->by_wday = config->by_date = config->by_time = 0;

    config->days_of_stat = 365;

    config->areas_count = 0;

    config->pkt_size = 10240;
    config->make_pkt = 0;
    config->pkt_orig_addr.zone = 0;
    config->pkt_orig_addr.net = 0;
    config->pkt_orig_addr.node = 0;
    config->pkt_orig_addr.point = 0;
    config->pkt_dest_addr.zone = 0;
    config->pkt_dest_addr.net = 0;
    config->pkt_dest_addr.node = 0;
    config->pkt_dest_addr.point = 0;
    for (j = 0; j<8; j++) config->pkt_password[j] = '\0';
    config->pkt_from = NULL;
    config->pkt_to = NULL;
    config->pkt_subj = NULL;
    config->pkt_origin = NULL;
    config->pkt_inbound = NULL;

    buff = malloc(BUFSIZE+1);

    while (fgets(line,BUFSIZE,fcfg))
    {
        i++;

        if (line == NULL) continue;

        j = strlen (line) - 1;
        while(j && (line[j] == '\r' || line[j] == '\n')) {line[j]='\0'; j--;}

        strcpy(buff,line);

        s0 = strtok(line,"\t \r\n");
        if (s0 == NULL) continue;
        if (*s0 == ';' || *s0 == '#') continue;

        if (!stricmp(s0,"By_Name"))
        {
            config->by_name = 1;
            continue;
        }

        if (!stricmp(s0,"By_Date"))
        {
            config->by_date = 1;
            continue;
        }

        if (!stricmp(s0,"By_WDay"))
        {
            config->by_wday = 1;
            continue;
        }

        if (!stricmp(s0,"By_Time"))
        {
            config->by_time = 1;
            continue;
        }

        if (!stricmp(s0,"By_From"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_from = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"By_To"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_to = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"By_FromTo"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_from_to = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"By_Total"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_total = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"By_Size"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_size = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"By_QPercent"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_qpercent = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"Subj_By_Total"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_subj = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"Stat_Period"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            if (*s1 == 'm')
            {
                s1++;
                config->days_of_stat = atoi(s1);
                if (!config->days_of_stat) print_err(buff,i);
                if (config->days_of_stat > 1000) print_err(buff,i);

                md = 0;
                n = t.tm_mon-1;

                for (j=1; j<config->days_of_stat; j++)
                {
                    md += days_in_months[n];
                    if (!n) n = 11; else n--;
                }

                config->days_of_stat = md + (t.tm_mday - 1);

            } else

                if (*s1 == 'w')
                {
                    s1++;
                    config->days_of_stat = atoi(s1);
                    if (!config->days_of_stat) print_err(buff,i);
                    if (config->days_of_stat > 1000) print_err(buff,i);
                    config->days_of_stat = ( (config->days_of_stat-1) * 7 ) + (wd - 1);
                } else

                {
                    config->days_of_stat = atoi(s1);
                    if (!config->days_of_stat) print_err(buff,i);
                    if (config->days_of_stat > 32768) print_err(buff,i);

                }

            continue;
        }

        if (!stricmp(s0,"Area"))
        {
            config->areas = srealloc(config->areas, sizeof(s_area)*(config->areas_count+1));

            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);

            config->areas[config->areas_count].name = (char *) smalloc(strlen(s1)+1);
            strcpy(config->areas[config->areas_count].name,s1);

            s2 = strtok(NULL,"\t \r\n");
            if (s2 == NULL) print_err(buff,i);

            config->areas[config->areas_count].type = 0;

            if (!stricmp(s2,"msg")) config->areas[config->areas_count].type = 1; else
                if (!stricmp(s2,"squish")) config->areas[config->areas_count].type = 2; else
                    if (!stricmp(s2,"jam")) config->areas[config->areas_count].type = 3;

            if (!config->areas[config->areas_count].type) print_err(buff,i);


            s3 = strtok(NULL,"\t \r\n");
            if (s3 == NULL) print_err(buff,i);

            config->areas[config->areas_count].path = (char *) smalloc(strlen(s3)+1);
            strcpy(config->areas[config->areas_count].path,s3);

            s4 = strtok(NULL,"\t \r\n");
            if (s4 == NULL) print_err(buff,i);

            config->areas[config->areas_count].out_file = (char *) smalloc(strlen(s4)+1);
            strcpy(config->areas[config->areas_count].out_file,s4);

            config->areas_count++;

            continue;
        }

        if (!stricmp(s0,"Subj_By_Total"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->by_subj = atoi(s1);
            continue;
        }

        if (!stricmp(s0,"Make_Pkt"))
        {
            config->make_pkt = 1;
            continue;
        }

        if (!stricmp(s0,"Pkt_Size"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->pkt_size = atoi(s1);
            if (!config->pkt_size) print_err(buff,i);
            if (config->pkt_size > 65536) print_err(buff,i);
            continue;
        }

        if (!stricmp(s0,"Pkt_Inbound"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);
            config->pkt_inbound = smalloc(strlen(s1)+2);
            strcpy(config->pkt_inbound,s1);
            if (config->pkt_inbound[strlen(config->pkt_inbound)-1] != '\\')
                strcat(config->pkt_inbound,"\\");
            continue;
        }

        if (!stricmp(s0,"Pkt_FromName"))
        {
            s1 = strtok(NULL,"\0");
            if (s1 == NULL) print_err(buff,i);
            while(*s1 && (*s1 == ' ' || *s1 == '\t')) s1++;
            j = strlen(s1)-1;
            while(j && (s1[j] == ' ' || s1[j] == '\t'))
            {
                s1[j] = '\0'; j--;
            }
            if (!*s1) print_err(buff,i);

            config->pkt_from = smalloc(strlen(s1)+1);
            strcpy(config->pkt_from,s1);

            continue;
        }

        if (!stricmp(s0,"Pkt_ToName"))
        {
            s1 = strtok(NULL,"\0");
            if (s1 == NULL) print_err(buff,i);
            while(*s1 && (*s1 == ' ' || *s1 == '\t')) s1++;
            j = strlen(s1)-1;
            while(j && (s1[j] == ' ' || s1[j] == '\t'))
            {
                s1[j] = '\0'; j--;
            }
            if (!*s1) print_err(buff,i);

            config->pkt_to = smalloc(strlen(s1)+1);
            strcpy(config->pkt_to,s1);

            continue;
        }

        if (!stricmp(s0,"Pkt_Subj"))
        {
            s1 = strtok(NULL,"\0");
            if (s1 == NULL) print_err(buff,i);
            while(*s1 && (*s1 == ' ' || *s1 == '\t')) s1++;
            j = strlen(s1)-1;
            while(j && (s1[j] == ' ' || s1[j] == '\t'))
            {
                s1[j] = '\0'; j--;
            }
            if (!*s1) print_err(buff,i);

            config->pkt_subj = smalloc(strlen(s1)+1);
            strcpy(config->pkt_subj,s1);

            continue;
        }

        if (!stricmp(s0,"Pkt_Origin"))
        {
            s1 = strtok(NULL,"\0");
            if (s1 == NULL) print_err(buff,i);
            while(*s1 && (*s1 == ' ' || *s1 == '\t')) s1++;
            j = strlen(s1)-1;
            while(j && (s1[j] == ' ' || s1[j] == '\t'))
            {
                s1[j] = '\0'; j--;
            }
            if (!*s1) print_err(buff,i);

            config->pkt_origin = smalloc(strlen(s1)+1);
            strcpy(config->pkt_origin,s1);

            continue;
        }

        if (!stricmp(s0,"Pkt_Password"))
        {
            s1 = strtok(NULL,"\0");
            if (s1 == NULL) print_err(buff,i);
            while(*s1 && (*s1 == ' ' || *s1 == '\t')) s1++;
            j = strlen(s1)-1;
            while(j && (s1[j] == ' ' || s1[j] == '\t'))
            {
                s1[j] = '\0'; j--;
            }
            if (!*s1) print_err(buff,i);

            strncpy(config->pkt_password,s1,8);

            continue;
        }

        if (!stricmp(s0,"Pkt_OrigAddress"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);

            if (!ScanNetAddr(&config->pkt_orig_addr, s1)) print_err(buff,i);

            continue;
        }

        if (!stricmp(s0,"Pkt_DestAddress"))
        {
            s1 = strtok(NULL,"\t \r\n");
            if (s1 == NULL) print_err(buff,i);

            if (!ScanNetAddr(&config->pkt_dest_addr, s1)) print_err(buff,i);

            continue;
        }

        fprintf(stderr,"Unknown keyword: \"%s\"\n",s0);

    } /* while */

    fclose(fcfg);

    free(buff);

    return config;

}

void free_items()
{
    unsigned long i;

    for (i = 0; i < gd_count; i++) nfree((char *)global_data[i].name);
    nfree(global_data);

    if (main_config->by_from)
    {
        for (i = 0; i < fd_count; i++) nfree((char *)from_data[i].name);
        nfree (from_data);
    }

    if (main_config->by_to)
    {
        for (i = 0; i < td_count; i++) nfree((char *)to_data[i].name);
        nfree (to_data);
    }

    if (main_config->by_from_to)
    {
        for (i = 0; i < ftd_count; i++) nfree((char *)from_to_data[i].name);
        nfree (from_to_data);
    }

    if (main_config->by_subj)
    {
        for (i = 0; i < sd_count; i++) nfree((char *)subj_data[i].name);
        nfree (subj_data);
    }

    nfree(date_data);

    if (main_config->by_time) nfree(time_data);

    if (main_config->by_size)
    {
        for (i = 0; i < szd_count; i++) nfree((char *)size_data[i].name);
        nfree (size_data);
    }

}

void free_config()
{
    unsigned long i;

    nfree((char *)main_config->pkt_from);
    nfree((char *)main_config->pkt_to);
    nfree((char *)main_config->pkt_subj);
    nfree((char *)main_config->pkt_origin);
    nfree((char *)main_config->pkt_inbound);

    for (i = 0; i < main_config->areas_count; i++)
    {
        nfree((char *)main_config->areas[i].name);
        nfree((char *)main_config->areas[i].path);
        nfree((char *)main_config->areas[i].out_file);
    }

    nfree(main_config->areas);
    nfree(main_config);

}

void print_write_err()
{
    fprintf(stderr,"Cannot write to pkt\n\n");
    exit(2);
}


int open_pkt()
{
    PKT_hdr ph;
    time_t tt = time(NULL);
    struct tm t;
    unsigned int i;
    char packet_name[1024];

    t = *localtime(&tt);

    global_msgid++;

    sprintf(packet_name,"%s%08lx%s",main_config->pkt_inbound,global_msgid,".pkt");

    if ((out_pkt = fopen(packet_name,"wb")) == NULL)
    {
        fprintf(stderr,"Cannot open file: \"%s\" for write\n\n",packet_name);
        exit(2);
    }

    ph.origNode = main_config->pkt_orig_addr.node;
    ph.destNode = main_config->pkt_dest_addr.node;
    ph.year = t.tm_year+1900;
    ph.month = t.tm_mon;
    ph.day = t.tm_mday;
    ph.hour = t.tm_hour;
    ph.minute = t.tm_min;
    ph.second = t.tm_sec;
    ph.baud = 0;
    ph.packettype = 2;
    ph.origNet = main_config->pkt_orig_addr.net;
    ph.destNet = main_config->pkt_dest_addr.net;
    ph.prodcode = 254;
    ph.serialno = 1;
    for (i=0; i<8; i++) ph.password[i] = main_config->pkt_password[i];
    ph.origZone = main_config->pkt_orig_addr.zone;
    ph.destZone = main_config->pkt_dest_addr.zone;
    ph.auxNet = ph.origNet;
    ph.CapValid = 256;
    ph.prodcode2 = 0;
    ph.serialno2 = 6;
    ph.CapWord = 1;
    ph.origZone2 = ph.origZone;
    ph.destZone2 = ph.destZone;
    ph.origPoint = main_config->pkt_orig_addr.point;
    ph.destPoint = main_config->pkt_dest_addr.point;
    ph.fill_36[0] = 'X';
    ph.fill_36[1] = 'P';
    ph.fill_36[2] = 'K';
    ph.fill_36[3] = 'T';

    fwrite(&ph,sizeof(ph),1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite("\0\0",2,1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    return 0;
}

int write_msg_hdr(int n)
{
    MSG_hdr mh;
    time_t tt = time(NULL);
    struct tm t;
    char yr[3],yy[10];
    char s[255];

    t = *localtime(&tt);

    mh.origNode = main_config->pkt_orig_addr.node;
    mh.destNode = main_config->pkt_dest_addr.node;
    mh.origNet = main_config->pkt_orig_addr.net;
    mh.destNet = main_config->pkt_dest_addr.net;
    mh.AttributeWord = 0;
    mh.cost = 0;

    sprintf(yy,"%04d",t.tm_year+1900);

    if (strlen(yy) == 4)
    {
        yr[0] = yy[2];
        yr[1] = yy[3];
        yr[2] = '\0';

    } else {

        yr[0] = '0';
        yr[1] = '0';
        yr[2] = '\0';

    }

    sprintf(mh.DateTime,"%02d %s %s  %02d:%02d:%02d",t.tm_mday,
            months_ab[t.tm_mon],yr,t.tm_hour,t.tm_min,isecs);

    isecs++;
    if (isecs > 59) isecs = 0; /* pseudo secs (anti dupes in squish) */

    fseek(out_pkt,-2L,SEEK_END);

    fwrite("\x02\0",2,1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite(&mh,sizeof(mh),1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite(main_config->pkt_to,strlen(main_config->pkt_to)+1,1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite(main_config->pkt_from,strlen(main_config->pkt_from)+1,1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite(main_config->pkt_subj,strlen(main_config->pkt_subj)+1,1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite("AREA:",5,1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite(main_config->areas[n].name,strlen(main_config->areas[n].name),1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    global_msgid++;

    if (main_config->pkt_orig_addr.point)
    {
        sprintf(s,"\r\x01MSGID: %d:%d/%d.%d %08lx",
                main_config->pkt_orig_addr.zone,main_config->pkt_orig_addr.net,
                main_config->pkt_orig_addr.node,main_config->pkt_orig_addr.point,
                global_msgid);

    } else {

        sprintf(s,"\r\x01MSGID: %d:%d/%d %08lx",
                main_config->pkt_orig_addr.zone,main_config->pkt_orig_addr.net,
                main_config->pkt_orig_addr.node,global_msgid);

    }

    fwrite(s,strlen(s),1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    sprintf(s,"\r\x01PID: %s\r",versionStr);

    fwrite(s,strlen(s),1,out_pkt);
    if (ferror(out_pkt)) print_write_err();

    return 0;
}

int write_buff_to_pkt(char *buff)
{
    fwrite (buff, strlen(buff), 1, out_pkt);
    if (ferror(out_pkt)) print_write_err();

    return 0;
}

int end_msg(unsigned char e)
{
    char s[255];

    if (e)
    {
        sprintf(s," -== Continued in next message ==-\r");

        fwrite (s, strlen(s), 1, out_pkt);
        if (ferror(out_pkt)) print_write_err();
    }

    sprintf(s,"\r\r--- %s",versionStr);

    fwrite (s, strlen(s), 1, out_pkt);
    if (ferror(out_pkt)) print_write_err();

    if (main_config->pkt_orig_addr.point)
    {
        sprintf(s,"\r * Origin: %s (%d:%d/%d.%d)",main_config->pkt_origin,
                main_config->pkt_orig_addr.zone,main_config->pkt_orig_addr.net,
                main_config->pkt_orig_addr.node,main_config->pkt_orig_addr.point);

    } else {

        sprintf(s,"\r * Origin: %s (%d:%d/%d)",main_config->pkt_origin,
                main_config->pkt_orig_addr.zone,main_config->pkt_orig_addr.net,
                main_config->pkt_orig_addr.node);
    }

    fwrite (s, strlen(s), 1, out_pkt);
    if (ferror(out_pkt)) print_write_err();

    sprintf(s,"\rSEEN-BY: %d/%d",main_config->pkt_orig_addr.net,
            main_config->pkt_orig_addr.node);

    fwrite (s, strlen(s), 1, out_pkt);
    if (ferror(out_pkt)) print_write_err();

    sprintf(s,"\r\x01PATH: %d/%d\r",main_config->pkt_orig_addr.net,
            main_config->pkt_orig_addr.node);

    fwrite (s, strlen(s), 1, out_pkt);
    if (ferror(out_pkt)) print_write_err();

    fwrite ("\0\0\0", 3, 1, out_pkt);
    if (ferror(out_pkt)) print_write_err();

    return 0;
}

int main(int argc,char *argv[])
{
    unsigned long i, msg_size = 0;
    int j;
    char ibuff[256];

    global_msgid = time(NULL);
    global_msgid %= 0xffffffff;

    versionStr = GenVersionStr( "areastat", VER_MAJOR, VER_MINOR, VER_PATCH,
                               VER_BRANCH, cvs_date );

    printf("%s\n", versionStr);

    if (argc > 2)
    {
        fprintf(stderr,"Too many actual parameters\n\n");
        argc = 0;
    }

    if (argc < 2)
    {
        fprintf(stderr,"\nUsage:\tareastat <config>\n");
        exit(1);
    }

    main_config = read_cfg(argv[1]);

    if (main_config->pkt_from == NULL) main_config->pkt_from = strdup(versionStr);
    if (main_config->pkt_to == NULL) main_config->pkt_to = strdup("All");
    if (main_config->pkt_subj == NULL) main_config->pkt_subj = strdup("Statistics");
    if (main_config->pkt_origin == NULL) main_config->pkt_origin = strdup("\0");
    if (main_config->pkt_inbound == NULL) main_config->pkt_inbound = strdup("\0");

    if (strlen(main_config->pkt_inbound) > 256)
    {
        fprintf(stderr,"Inbound path too long\n\n");
        exit(2);
    }

    if (strlen(main_config->pkt_origin) > 50)
    {
        fprintf(stderr,"Origin too long\n\n");
        exit(2);
    }

    for (i = 0; i < main_config->areas_count; i++)
    {

        if ((current_std = fopen(main_config->areas[i].out_file,"wb")) == NULL)
        {
            fprintf(stderr,"Cannot open file: \"%s\"\n\n",main_config->areas[i].out_file);
            exit(2);
        }

        reading_base(i);

        print_summary_statistics(i);
        if (main_config->by_name) print_statistics_by_name();
        if (main_config->by_from) print_statistics_by_from();
        if (main_config->by_to) print_statistics_by_to();
        if (main_config->by_from_to) print_statistics_by_from_to();
        if (main_config->by_total) print_statistics_by_total();
        if (main_config->by_size) print_statistics_by_size();
        if (main_config->by_qpercent) print_statistics_by_qpercent();
        if (main_config->by_subj) print_statistics_by_subj();
        if (main_config->by_date) print_statistics_by_date();
        if (main_config->by_wday) print_statistics_by_wdays();
        if (main_config->by_time) print_statistics_by_time();

        fprintf(current_std,"\n Statistics created by %s",versionStr);
        fprintf(current_std,"\n Written by Dmitry Rusov (C) 2000, The Husky project (C) 2004\n");

        fclose(current_std);

        if (!main_config->make_pkt)
        {
            free_items();
            continue;
        }

        open_pkt();

        if ((current_std = fopen(main_config->areas[i].out_file,"rb")) == NULL)
        {
            fprintf(stderr,"Cannot open file: \"%s\"\n\n",main_config->areas[i].out_file);
            exit(2);
        }

        isecs = 0;

        write_msg_hdr(i);

        if ((current_std = fopen(main_config->areas[i].out_file,"rb")) == NULL)
        {
            fprintf(stderr,"Cannot open file: \"%s\"\n\n",main_config->areas[i].out_file);
            exit(2);
        }

        msg_size = 0;

        while (fgets(ibuff,255,current_std))
        {
            if (ibuff != NULL && * ibuff)
            {
                if (ibuff[0] == ' ' && ibuff[1] == '-' && ibuff[2] == '-'
                    && ibuff[3] == '-' && msg_size > main_config->pkt_size)
                {
                    end_msg(1);
                    write_msg_hdr(i);
                    msg_size = 0;
                }

                j = strlen(ibuff)-1;
                if (j >= 0 && ibuff[j] == '\n') ibuff[j] = '\r';

                write_buff_to_pkt(ibuff);
                msg_size += strlen(ibuff);
            }
        }
        end_msg(0);
        fclose(current_std);
        fclose(out_pkt);
        free_items();

    } /* for */
    free_config();
    fprintf(stderr,"\n\nDone!\n");
    return 0;
}
