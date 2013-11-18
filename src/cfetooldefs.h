
#define DAILY 1
#define WEEKLY 2
#define YEARLY 3
#define MONDAY_MORNING 345600
#define JANUARY_FIRST 63072000
#define ONE_YEAR 31536000
#define ONE_DAY 86400
#define ONE_WEEK 604800

struct Average
   {
   double expect;
   double var;
   };

char *GenTimeKey2 (time_t now, int dbtype);
char *ConvTimeKey2 (char *str, int dbtype);
