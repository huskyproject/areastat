#include <stdio.h>
#include <fidoconf/fidoconf.h>
#include <smapi/msgapi.h>

typedef struct item {
    char *label;
    unsigned int value;
} item;

void addItem(item **list, int *list_max, char *label, unsigned int value)
{
    item *new_list;

    new_list = realloc(*list, sizeof(item)*((*list_max)+1+1));
    if (!new_list) return;

    (*list_max)++;
    *list = new_list;
    (*list)[*list_max].label=(char *)strdup(label);
    (*list)[*list_max].value=value;
}

int searchItem(item *list, int list_max, char *label)
{
    int i;

    if (!list) return -1;

    for(i=0;i<=list_max;i++) {
        if (!strcoll(label, list[i].label))
            return i;
    }
    return -1;
}

void incItem(item **list, int *list_max, char *label)
{
    int i=-1;

    if (*list)
        i=searchItem(*list, *list_max, label);

    if(i==-1) {
        i++;
        addItem(list, list_max, label, 1);
    }
    (*list)[i].value++;
}

int compareItemsValues(const void *i1, const void *i2)
{
    if (((item *)i1)->value > ((item *)i2)->value)
        return -1;
    else if (((item *)i1)->value < ((item *)i2)->value)
        return 1;
    else
        return 0;
}

void printItems(item *list, int list_max, char *title)
{
    int i;

    printf("****** %s ******\n", title);
    for (i=0;i<=list_max && i < 10;i++)
        printf("%s: %lu\n", list[i].label, list[i].value);
    printf("\n");
}

int main(int argc, char **argv)
{
    ps_fidoconfig config;
    s_area *fc_area;
    struct _minf m;     /* Declare structure for MSGAPI info */
    HAREA area;         /* Declare area structure (echoarea, netmailarea
                           or localarea - its is unimportant) */
    HMSG msg;           /* Declare structure for message store */
    XMSG xmsg;          /* Another structure for message store */
    char *text = NULL;
    dword  textLen = 0;
    unsigned long highMsg;
    unsigned long i;

    item *fromList=NULL;
    int fromList_max=-1;
    item *toList=NULL;
    int toList_max=-1;
    item *subjectList=NULL;
    int subjectList_max=-1;

    if (argc<2) {
        fprintf(stderr, "Area name is not spesified. Please do it.\n");
        return 1;
    }

    if(!(config=readConfig(NULL))) {
        fprintf(stderr, "Can't open config file\n");
        return 1;
    }
    m.req_version = 2;  /* Set smapi version number */

              /* vvvvvv  config readed before this line or use alternate value/code */
    m.def_zone = config->addr[0].zone;
    if (MsgOpenApi(&m)!= 0) {     /* Open MSGAPI handler call (initialise MSGAPI) */
        printf("MsgOpenApi Error.\n");
        exit(1);
    }

    if (!(fc_area = getArea(config, argv[1]))) {
        fprintf(stderr, "Area %s not found.\n", argv[1]);
        return 1;
    }
    /* Open area handler call (initialise area, read some info from disk file) */
    if (!(area = MsgOpenArea((byte *) fc_area->fileName, MSGAREA_NORMAL, fc_area->msgbType))) {
        fprintf(stderr, "Can't open area %s: %s.\n", fc_area->fileName, strmerr(msgapierr));
        return 1;
    }

    highMsg=MsgGetHighMsg(area);

    for (i=highMsg-1000;i<=highMsg;i++) {
        if (!(msg = MsgOpenMsg(area, MOPEN_READ, i))) {
            fprintf(stderr, "Can't open msg %lu.\n", i);
            return 1;
        }
        /* Read message */
        MsgReadMsg(msg, &xmsg, 0, 0, NULL, 0, NULL);

/*        fprintf(stderr, "%s|%s|%s\n", xmsg.from, xmsg.to, xmsg.subj); */

        /* process its header */
        incItem(&fromList,    &fromList_max,    xmsg.from);
        incItem(&toList,      &toList_max,      xmsg.to);
        incItem(&subjectList, &subjectList_max, xmsg.subj);

        /* Close message: deinit handler call (flush buffers, unlock file, release memory) */
        if (MsgCloseMsg(msg)) {
            fprintf(stderr, "Couldn't close message %u: %s\n", i, strmerr(msgapierr));
            break;
        }
    }

    qsort(fromList,    fromList_max,    sizeof(item), compareItemsValues);
    qsort(toList,      toList_max,      sizeof(item), compareItemsValues);
    qsort(subjectList, subjectList_max, sizeof(item), compareItemsValues);
    printItems(fromList,      fromList_max,      "Statistics by 'From' field:");
    printItems(toList,        toList_max,        "Statistics by 'To' field:");
    printItems(subjectList,   subjectList_max,   "Statistics by 'Subject' field:");

    /* Close area: deinit handler call (close file, release memory) */
    MsgCloseArea(area);

    /* Close MSGAPI: deinit handler call (release memory) */
    MsgCloseApi();

    disposeConfig(config);
    return 0;
}
