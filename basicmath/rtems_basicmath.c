#include <sched.h>
#include <bsp.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <rtems.h>
#include <math.h>
#include <stdint.h>
#include "snipmath.h"

#define  PRINT  0 // Print Switch

#define PROCESSORS 2

#define CUBIC 1
#define ISQRT 2
#define ULSQRT 3
#define DEGTORAD 4
#define RADTODEG 5

// Switch to do large or small calculations.
#define     LARGE_DATA_SET  1
#define     SMALL_DATA_SET  2

#define     ONE_KILO_NUMBERS 1024
#define     ONE_MEGA_NUMBERS 1024*1024
#define     ONE_GIGA_NUMBERS 1024*1024*1024

pthread_attr_t string_attr;
pthread_t workers[PROCESSORS];
pthread_t system_alive;
struct sched_param param;

int math_operation;
int data_set_type;
int no_workers;

typedef struct Ilimits{ // Limits of ineger type
	int _start;
	int _end;
	int _offset;
}ilimit;

typedef struct ULlimits{ // Limits of unsigned long type
	unsigned long _start;
	unsigned long _end;
	unsigned long _offset;
}ullimit;

typedef struct DoubleLimits{  // Limits of double type
	double _start;
	double _end;
	double _offset;
}dlimit;

static ilimit  _ilim,_ilimParams[PROCESSORS];
static ullimit _ullim,_ullimParams[PROCESSORS];
static dlimit  _dlim1,_dlim2,_dlimParams1[PROCESSORS],_dlimParams2[PROCESSORS];
static dlimit  _adlim,_bdlim,_cdlim,_ddlim,_adlimParams[PROCESSORS];  // double type limits for variable's a,b,c,d

void initializeLimits(void);
void divide_into_sub_tasks(void);
void *doWork(void*);
void *alive(void*);

void *POSIX_Init(void) {
	int index;

	math_operation=2;
	no_workers=2;
	data_set_type=1;

	printf("Initiated....\n");

	if (no_workers > PROCESSORS){
		printf("ERROR: Number of worker should be no more than 8\n");
		exit(0);
	}

	initializeLimits();

	divide_into_sub_tasks();

	pthread_attr_init(&string_attr);
	pthread_attr_setdetachstate(&string_attr,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setschedpolicy(&string_attr, SCHED_FIFO);

	for(index=0;index<no_workers;index++){
		int *arg = malloc(sizeof(*arg));
		*arg=index;
		param.sched_priority = sched_get_priority_max(SCHED_FIFO) - index;
		pthread_attr_setschedparam(&string_attr, &param);
		if( pthread_create(&workers[index],&string_attr,doWork,arg) ||
			pthread_setschedparam(workers[index], SCHED_FIFO, &param) ){
			printf("error on creating worker %d...\n",index);
			exit(1);
		}
		else{
			printf("Worker %d created.\n",index);
		}
	}

//	param.sched_priority = sched_get_priority_min(SCHED_FIFO);
	param.sched_priority = sched_get_priority_max(SCHED_FIFO) - index;
	pthread_attr_setschedparam(&string_attr, &param);
	if( pthread_create(&system_alive,&string_attr,alive,NULL) ||
		pthread_setschedparam(system_alive, SCHED_FIFO, &param) ){
		printf("error on creating system_alive...\n");
		exit(1);
	}
	else{
		printf("system_alive created.\n",index);
	}

	for(index=0;index<no_workers;index++)
		pthread_join(workers[index],NULL);
	pthread_join(system_alive,NULL);

	printf("Finished Working\n");
	exit(0);
}

void initializeLimits(void){
	if (data_set_type==LARGE_DATA_SET){
		//Initialization of integer type limits
		_ilim._start=0;
		_ilim._end=  1*ONE_GIGA_NUMBERS;
		_ilim._offset=1;

		//Initialization of unsigned long type limits
		_ullim._start=1072497001;
		_ullim._end=1072497001+ (1*ONE_GIGA_NUMBERS);
		_ullim._offset=1;

		//Initialization of double type limits
		_adlim._start=1.0,_adlim._end=16.0,_adlim._offset=1e-10;
		_bdlim._start=10.0,_bdlim._end=10.0,_bdlim._offset=0.25;
		_cdlim._start=5.0,_cdlim._end=15.0,_cdlim._offset=0.61;
		_ddlim._start=-1.0,_ddlim._end=-5.0,_ddlim._offset=0.451;

		_dlim1._start=0.0;
		_dlim1._end=360.0;
		_dlim1._offset=1e-9;

		_dlim2._start=0.0;
		_dlim2._end=(6 * PI + 1e-6);
		_dlim2._offset=(PI / 1e11);

	} else if (data_set_type==SMALL_DATA_SET){
		//Initialization of integer type limits
		_ilim._start=0;
		_ilim._end=1*ONE_MEGA_NUMBERS;
		_ilim._offset=1;

		//Initialization of unsigned long type limits
		_ullim._start=1072497001;
		_ullim._end=  1072497001 +(1*ONE_MEGA_NUMBERS);
		_ullim._offset=1;

		//Initialization of double type limits
		_adlim._start=1.0,_adlim._end=10.0,_adlim._offset=1e-9;
		_bdlim._start=10.0,_bdlim._end=10.0,_bdlim._offset=0.25;
		_cdlim._start=5.0,_cdlim._end=15.0,_cdlim._offset=0.61;
		_ddlim._start=-1.0,_ddlim._end=-5.0,_ddlim._offset=0.451;

		_dlim1._start=0.0;
		_dlim1._end=360.0;
		_dlim1._offset=1e-8;

		_dlim2._start=0.0;
		_dlim2._end=(6 * PI + 1e-6);
		_dlim2._offset=(PI /1e10);
	}
}//end-initializeLimits

void solveCubicEquations(int workerID){

	double a1,b1,c1,d1;
	double x[3];
	int solutions, index;

	printf("********* SOLVE CUBIC EQUATIONS : Worker %d  ***********\n",workerID+1);

	for(a1=_adlimParams[workerID]._start;a1<_adlimParams[workerID]._end;a1+=_adlim._offset) {
		for(b1=_bdlim._start;b1>_bdlim._end;b1-=_bdlim._offset) {
			for(c1=_cdlim._start;c1<_cdlim._end;c1+=_cdlim._offset) {
				for(d1=-_ddlim._start;d1>_ddlim._end;d1-=_ddlim._offset) {
					SolveCubic(a1, b1, c1, d1, &solutions, x);
					if (PRINT){
						printf("Solutions:");
						for(index=0;index<solutions;index++)
							printf(" %f",x[index]);
						printf("\n");
					}//endif
				}// end-for
			}//end-for
		}//end-for
	}//end-for
}

void calcISqrt(int workerID){
	int index;
	struct int_sqrt q;

	printf("********* CALCULATE INTEGER SQR ROOTS : Worker %d  ***********\n",workerID+1);
	for(index = _ilimParams[workerID]._start; index < _ilimParams[workerID]._end; index+=_ilim._offset){
		usqrt(index, &q);
		//remainder differs on some machines
		if (PRINT)
		printf("sqrt(%3d) = %2d\n",index, q.sqrt);
	}//end-for
}

void calcULSqrt(int workerID){
	unsigned long l = 0x3fed0169L;
	struct int_sqrt q;

	printf("********* CALCULATE LONG SQR ROOTS : Worker %d  ***********\n",workerID+1);
	for(l =_ullimParams[workerID]._start ; l <_ullimParams[workerID]._end; l+=_ullim._offset){
		usqrt(l, &q);
		if (PRINT)
			printf("sqrt(%lX) = %X\n", l, q.sqrt);
	}
}

void degToRadianConv(int workerID){
	double X;
	printf("********* PERFORM DEGREE TO RADIAN ANGLE CONVERSION : Worker %d  ***********\n",workerID+1);
	/* convert some rads to degrees */
	for (X =_dlimParams1[workerID]._start; X <=_dlimParams1[workerID]._end; X +=_dlim1._offset){
		#if PRINT
			printf("%3.0f degrees = %.12f radians\n", X, ret);
		#else
			deg2rad(X);
		#endif
	}
}

void radianToDegConv(int workerID){
	double X;
	printf("********* PERFORM RADIAN TO DEGREE ANGLE CONVERSION : Worker %d  ***********\n",workerID+1);
	for (X =_dlimParams2[workerID]._start; X <=_dlimParams2[workerID]._end; X +=_dlim2._offset){
		#if PRINT
			printf("%.12f radians = %3.0f degrees\n", X, rad2deg(X));
		#else
			rad2deg(X);
		#endif
	}
}

void *doWork(void *workerID){
	int id = *((int *) workerID);

	printf("Worker %d started work.\n",id+1);  

	//Each thread performs following five tasks according to assigned work.
	if (math_operation ==CUBIC )
		solveCubicEquations(id);
	else if (math_operation ==ISQRT)
		calcISqrt(id);
	else if (math_operation ==ULSQRT)
		calcULSqrt(id);
	else if (math_operation ==DEGTORAD)
		degToRadianConv(id);
	else if (math_operation ==RADTODEG)
		radianToDegConv(id);
	else
		printf("Undefined math operation\n");	

	if (PRINT)
		printf("Worker %d has completed work.\n",id);

	return NULL;
}

void *alive(void *unused){
	while(1){
		printf("Ticks since boot: %d\n", (int)rtems_clock_get_ticks_since_boot());
		rtems_task_wake_after(200);
	}
}

void divide_into_sub_tasks(){
	int i,temp;
	int isindex;
	unsigned long ulsindex,ulTemp;
	double dsindex1,dsindex2,adsindex;

	adsindex = _adlim._start;
	isindex = _ilim._start;
	ulsindex = _ullim._start;

	dsindex1 = _dlim1._start;
	dsindex2 = _dlim2._start;

	// Total long numbers
	ulTemp = _ullim._end-_ullim._start;

	for(i=0;i<no_workers;i++) {
		_adlimParams[i]._start = adsindex;
		_adlimParams[i]._end = _adlimParams[i]._start+(_adlim._end/no_workers);
		adsindex += (_adlim._end/no_workers);

		_ilimParams[i]._start = isindex;
		_ilimParams[i]._end = _ilimParams[i]._start+(_ilim._end/no_workers);
		isindex += (_ilim._end/no_workers);

		_ullimParams[i]._start = ulsindex;
		_ullimParams[i]._end = _ullimParams[i]._start+(ulTemp/no_workers);
		ulsindex += (ulTemp/no_workers);

		_dlimParams1[i]._start = dsindex1;
		_dlimParams1[i]._end = _dlimParams1[i]._start+(_dlim1._end/no_workers);
		dsindex1 += (_dlim1._end/no_workers);

		temp = _dlim2._end;
		_dlimParams2[i]._start = dsindex2;
		_dlimParams2[i]._end = _dlimParams2[i]._start+(temp/no_workers);
		dsindex2 += (temp/no_workers);
	}

	// It handles even and odd number total work.
	_ilimParams[i-1]._end = _ilimParams[i-1]._end+(_ilim._end%no_workers);
	_ullimParams[i-1]._end = _ullimParams[i-1]._end+(_ullim._end%no_workers);
}

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_SMP_APPLICATION
#define CONFIGURE_SMP_MAXIMUM_PROCESSORS 2

#define CONFIGURE_MAXIMUM_POSIX_THREADS              10
#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES  10
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES              10
#define CONFIGURE_POSIX_INIT_THREAD_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>


