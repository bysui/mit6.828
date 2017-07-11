#include "types.h"
#include "user.h"
#include "date.h"

int
main(int argc, char *argv[])
{
	struct rtcdate r;

	if (date(&r)) {
		printf(2, "date failed\n");
		exit();
	}
	
  	printf(2, "%d-%d-%d\t%d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);	
	exit();
}
