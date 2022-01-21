#include "local.h"

// Shared memory for officer queues.
struct shmque
{
	// variable to store current passengers count in the queue.
	int queue_count;
	// Variable to store the queue type.
	int queue_type;
};

// Struture of message to be sent in queue.
struct msg_buffer
{
	// Variable to store passport validity.
	bool isPassportValid;
	// Variable to store passengers shared memory id.
	int shmId;
};

// Shared memory structure of passengers.
struct shmpass
{
	// Variable to store if the passport is processed or not.
	int isProcessed;
	// Data store if the patient lost patience and left queue.
	bool isTimedOut;
};

// Shared memory structure to store exit variables.
struct exitDb
{
	// Variable to store total passengers allowed.
	int totalCount;
	// Variable to store total passengers denied.
	int totalDenied;
	// Variable to store total passengers left queue.
	int totalLeft;
};

// Shared memory structure to store hall data.
struct hallDb
{
	// Variable to store total passengers in the hall.
	int total;
	// Variable to store total Palestinians approved.
	int totalApprovedP;
	// Variable to store total Jordanians approved.
	int totalApprovedJ;
	// Variable to store total Foreigners approved.
	int totalApprovedF;
};

// Shared memory structure to store denied data.
struct deniedDb
{
	// Variable to store total Palestinians denied.
	int totalDeniedP;
	// Variable to store total Jordanians denied.
	int totalDeniedJ;
	// Variable to store total Foreigners denied.
	int totalDeniedF;
	// Variable to store total Palestinians left queue.
	int totalLeftP;
	// Variable to store total Jordanians left queue.
	int totalLeftJ;
	// Variable to store total Foreigners left queue.
	int totalLeftF;
};

// Shared memory structure to store bus data
struct busDb
{
	// Variable to store bus ID
	int busId;
	// Variable to store bus running status.
	int busStatus;
	// Variable to store passengers count in the bus.
	int count;
};

// Variables to store config paramerters from the file.
int no_crossing_points_p;
int no_crossing_points_j;
int no_crossing_points_f;
int no_officers;
int no_buses;
int bus_sleep_l;
int bus_sleep_h;
int passengers_arrival_rate;
int passenger_timeout_lower;
int passenger_timeout_higher;
int officer_timeout_lower;
int officer_timeout_higher;
int bus_capacity;
int bus_count;
int bus_trip_delay;
int hall_max;
int hall_min;
int max_allowed;
int max_denied;
int max_left;

void config_parser(void);
void create_officers();
int get_random_value(int, int);
void create_buses();
void print_data();

int main()
{
	int pid;
	int msgId;
	int shmId, shmHId, shmDId, exitId;
	int val = 0;
	int passengerType;
	int lRange;
	int hRange;
	int min = 0;
	int minIndex = 0;
	int timeout = 0;
	struct shmque *shmq;
	struct shmpass *shmpass;
	struct hallDb *hall;
	struct deniedDb *denied;
	struct exitDb *ex;
	struct msg_buffer msg;
	struct timeval tv, tv_start, tv_end;

	config_parser();

	usleep(3000000);

	if (no_officers != (no_crossing_points_p + no_crossing_points_j + no_crossing_points_f))
	{
		fprintf(stderr, "Officers combination error\n");
		exit(-1);
	}
	shmHId = shmget(HALL_KEY, sizeof(struct hallDb), 0644 | IPC_CREAT);
	// Initialize hall data values.
	hall = shmat(shmHId, NULL, 0);
	if (hall == (void *)-1)
	{
		fprintf(stderr, "Shared memory attach error !\n");
	}
	hall->total = 0;
	hall->totalApprovedP = 0;
	hall->totalApprovedJ = 0;
	hall->totalApprovedF = 0;
	if (shmdt(hall) == -1)
	{
		fprintf(stderr, "shmdt\n");
	}
	// Initialize denied passengers values.
	shmDId = shmget(DENIED_KEY, sizeof(struct deniedDb), 0644 | IPC_CREAT);
	denied = shmat(shmDId, NULL, 0);
	if (denied == (void *)-1)
	{
		fprintf(stderr, "Shared memory attach error !\n");
	}
	denied->totalDeniedP = 0;
	denied->totalDeniedJ = 0;
	denied->totalDeniedF = 0;
	denied->totalLeftP = 0;
	denied->totalLeftJ = 0;
	denied->totalLeftF = 0;

	if (shmdt(denied) == -1)
	{
		fprintf(stderr, "shmdt\n");
	}
	// Intialize exit data values.
	exitId = shmget(EXIT_KEY, sizeof(struct exitDb), 0644 | IPC_CREAT);
	ex = shmat(exitId, NULL, 0);
	if (ex == (void *)-1)
	{
		fprintf(stderr, "Shared memory attach error !\n");
	}
	ex->totalCount = 0;
	ex->totalDenied = 0;
	ex->totalLeft = 0;
	if (shmdt(ex) == -1)
	{
		fprintf(stderr, "shmdt\n");
	}

	create_officers();

	create_buses();

	print_data();

	// Infinitely create passenger process
	while (true)
	{
		ex = shmat(exitId, NULL, 0);
		if (ex == (void *)-1)
		{
			fprintf(stderr, "Shared memory attach error !\n");
		}
		// Check exit condition
		if (ex->totalCount >= max_allowed ||
			ex->totalDenied >= max_denied ||
			ex->totalLeft >= max_left)
		{
			if (shmdt(ex) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			fprintf(stderr, "Simulation Ends\n");
			exit(0);
		}
		if (shmdt(ex) == -1)
		{
			fprintf(stderr, "shmdt\n");
		}
		// Create passenger child process
		pid = fork();
		if (pid == 0)
		{
			// Child process
			passengerType = get_random_value(1, no_officers);
			// Logic to find range of the officer queue the passenger has to wait
			// based on the passenger type.
			if (passengerType <= no_crossing_points_p)
			{
				lRange = 0;
				hRange = no_crossing_points_p;
				passengerType = QUEUE_P;
			}
			else if (passengerType > no_crossing_points_p &&
					 passengerType <= (no_crossing_points_p + no_crossing_points_j))
			{
				lRange = no_crossing_points_p;
				hRange = no_crossing_points_p + no_crossing_points_j;
				passengerType = QUEUE_J;
			}
			else
			{
				lRange = no_crossing_points_p + no_crossing_points_j;
				hRange = no_officers;
				passengerType = QUEUE_F;
			}
			shmId = shmget(KEY + lRange, sizeof(struct shmque), 0644 | IPC_CREAT);
			shmq = shmat(shmId, NULL, 0);
			if (shmq == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			// Logic to find the officer queue which has less number of people waiting.
			min = shmq->queue_count;
			minIndex = lRange;
			if (shmdt(shmq) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			for (int i = lRange + 1; i < hRange; i++)
			{
				shmId = shmget(KEY + i, sizeof(struct shmque), 0644 | IPC_CREAT);
				shmq = shmat(shmId, NULL, 0);
				if (shmq == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				if (shmq->queue_count <= min)
				{
					// Officer queue with lowest number of passengers is found.
					min = shmq->queue_count;
					minIndex = i;
				}
				if (shmdt(shmq) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}
			}
			shmId = shmget(KEY + minIndex, sizeof(struct shmque), 0644 | IPC_CREAT);
			shmq = shmat(shmId, NULL, 0);
			if (shmq == (void *)-1)
			{
				fprintf(stderr, " Shared memory attach error !\n");
			}
			// Update the queue count.
			shmq->queue_count++;
			if (shmdt(shmq) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			// Randomly generate passport validity status.
			val = get_random_value(1, 100);
			if (val % 2 == 0)
			{
				msg.isPassportValid = true;
			}
			else
			{
				msg.isPassportValid = false;
			}
			// Create a shared memory for the passenger. Use current time as shared memory key.
			gettimeofday(&tv, NULL);
			shmId = shmget(tv.tv_usec, sizeof(struct shmpass), 0644 | IPC_CREAT);
			shmpass = shmat(shmId, NULL, 0);
			if (shmpass == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			// Initialize passenger shared memory varible.
			shmpass->isProcessed = WAITING;
			shmpass->isTimedOut = false;
			if (shmdt(shmpass) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			// Update the shared memory id in the message to be sent to the officer.
			msg.shmId = shmId;
			msgId = msgget(KEY + minIndex, 0666 | IPC_CREAT);
			// Send the message to the officer queue.
			val = msgsnd(msgId, &msg, sizeof(msg), 0);
			// Passenger wait time is obtained.
			timeout = get_random_value(passenger_timeout_lower, passenger_timeout_higher);
			// Passenger starts to wait for certain amount of time.
			gettimeofday(&tv_start, NULL);
			do
			{
				shmpass = shmat(shmId, NULL, 0);
				if (shmpass == (void *)-1)
				{
					fprintf(stderr, " Shared memory attach error !\n");
				}
				else
				{
					// If the passport is approved break the waiting loop
					if (shmpass->isProcessed == PROCESSED)
					{
						if (shmdt(shmpass) == -1)
						{
							fprintf(stderr, " shmdt\n");
						}
						shmctl(shmId, IPC_RMID, NULL);
						break;
					}
					// If the passport is denied break the waiting loop.
					else if (shmpass->isProcessed == DENIED)
					{
						if (shmdt(shmpass) == -1)
						{
							fprintf(stderr, " shmdt\n");
						}
						shmctl(shmId, IPC_RMID, NULL);
						// Update the denied passengers count in respective shared memory.
						denied = shmat(shmDId, NULL, 0);
						if (denied == (void *)-1)
						{
							fprintf(stderr, "Shared memory attach error !\n");
						}
						ex = shmat(exitId, NULL, 0);
						if (ex == (void *)-1)
						{
							fprintf(stderr, "Shared memory attach error !\n");
						}
						ex->totalDenied++;
						if (shmdt(ex) == -1)
						{
							fprintf(stderr, "shmdt\n");
						}
						if (passengerType == QUEUE_P)
						{
							denied->totalDeniedP++;
						}
						else if (passengerType == QUEUE_J)
						{
							denied->totalDeniedJ++;
						}
						else
						{
							denied->totalDeniedF++;
						}
						if (shmdt(denied) == -1)
						{
							fprintf(stderr, " shmdt\n");
						}
						break;
					}
					else
					{
						if (shmdt(shmpass) == -1)
						{
							fprintf(stderr, " shmdt\n");
						}
					}
				}
				gettimeofday(&tv_end, NULL);
				// Calculate the time difference. If the differnce exceed the timeout amount
				// break the loop.
				if ((tv_end.tv_sec - tv_start.tv_sec) >= timeout)
				{
					shmpass = shmat(shmId, NULL, 0);
					if (shmpass == (void *)-1)
					{
						fprintf(stderr, "   Shared memory attach error !\n");
					}
					shmpass->isTimedOut = true;
					if (shmdt(shmpass) == -1)
					{
						fprintf(stderr, " shmdt\n");
					}
					denied = shmat(shmDId, NULL, 0);
					if (denied == (void *)-1)
					{
						fprintf(stderr, "Shared memory attach error !\n");
					}
					ex = shmat(exitId, NULL, 0);
					if (ex == (void *)-1)
					{
						fprintf(stderr, "Shared memory attach error !\n");
					}
					// Update the passengers left count in the respective
					// shared memory.
					ex->totalLeft++;
					if (shmdt(ex) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
					if (passengerType == QUEUE_P)
					{
						denied->totalLeftP++;
					}
					else if (passengerType == QUEUE_J)
					{
						denied->totalLeftJ++;
					}
					else
					{
						denied->totalLeftF++;
					}
					if (shmdt(denied) == -1)
					{
						fprintf(stderr, " shmdt\n");
					}
					break;
				}
				usleep(1000000);
			} while ((tv_end.tv_sec - tv_start.tv_sec) <= timeout); // Continue waiting until the wait time exceeds the timeout.
			exit(0);												// Kill the passenger process.
		}
		usleep(passengers_arrival_rate * 1000000);
	}
	return 0;
}

/*
  Function name : config_parser
  Args : None
  Returns : None

  Description : Function read the config file and update the corresponding parameters.
*/
void config_parser(void)
{
	FILE *fp;
	char chunk[100];
	int i;
	i = 0;
	memset(chunk, '\0', 100);
	fp = fopen(CONFIG_FILE, "r");

	if (fp == NULL)
	{
		fprintf(stderr, "Unable to open file\n");
		exit(1);
	}

	while (fgets(chunk, sizeof(chunk), fp) != NULL)
	{
		if (i == 0)
			no_crossing_points_p = atoi(&chunk[strlen("NUMBER_CROSSING_POINTS_P=")]);
		else if (i == 1)
			no_crossing_points_j = atoi(&chunk[strlen("NUMBER_CROSSING_POINTS_J=")]);
		else if (i == 2)
			no_crossing_points_f = atoi(&chunk[strlen("NUMBER_CROSSING_POINTS_F=")]);
		else if (i == 3)
			no_officers = atoi(&chunk[strlen("NUMBER_OFFICERS=")]);
		else if (i == 4)
			no_buses = atoi(&chunk[strlen("NUMBER_BUSES=")]);
		else if (i == 5)
			bus_sleep_l = atoi(&chunk[strlen("BUS_SLEEP_PERIOD_LOWER=")]);
		else if (i == 6)
			bus_sleep_h = atoi(&chunk[strlen("BUS_SLEEP_PERIOD_HIGHER=")]);
		else if (i == 7)
			passengers_arrival_rate = atoi(&chunk[strlen("PASSENGER_ARRIVAL_RATE=")]);
		else if (i == 8)
			passenger_timeout_lower = atoi(&chunk[strlen("PASSENGER_TIMEOUT_LOWER=")]);
		else if (i == 9)
			passenger_timeout_higher = atoi(&chunk[strlen("PASSENGER_TIMEOUT_HIGHER=")]);
		else if (i == 10)
			officer_timeout_lower = atoi(&chunk[strlen("OFFICER_TIMEOUT_LOWER=")]);
		else if (i == 11)
			officer_timeout_higher = atoi(&chunk[strlen("OFFICER_TIMEOUT_HIGHER=")]);
		else if (i == 12)
			bus_capacity = atoi(&chunk[strlen("BUS_CAPACITY=")]);
		else if (i == 13)
			bus_count = atoi(&chunk[strlen("BUS_COUNT=")]);
		else if (i == 14)
			bus_trip_delay = atoi(&chunk[strlen("BUS_TRIP_TIME=")]);
		else if (i == 15)
			hall_max = atoi(&chunk[strlen("HALL_MAX=")]);
		else if (i == 16)
			hall_min = atoi(&chunk[strlen("HALL_MIN=")]);
		else if (i == 17)
			max_allowed = atoi(&chunk[strlen("MAX_ALLOWED=")]);
		else if (i == 18)
			max_denied = atoi(&chunk[strlen("MAX_DENIED=")]);
		else if (i == 19)
			max_left = atoi(&chunk[strlen("MAX_LEFT=")]);

		memset(chunk, '\0', 100);
		i++;
	}
	fclose(fp);
}

/*
  Function name : create_officers
  Args : None
  Returns : None

  Description : Function to create processes representing officers.
*/
void create_officers()
{
	int pid, hallCount;
	int shmId, msgId, exitId, shmHId;
	struct shmque *shmq;
	struct msg_buffer msg;
	struct exitDb *ex;
	struct shmpass *shmpass;
	struct hallDb *hall;
	int timeout = 0;

	exitId = shmget(EXIT_KEY, sizeof(struct exitDb), 0644 | IPC_CREAT);
	shmHId = shmget(HALL_KEY, sizeof(struct hallDb), 0644 | IPC_CREAT);

	// Loop to create officers.
	for (int i = 0; i < no_officers; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			// Create shared memory for the officer.
			shmId = shmget(KEY + i, sizeof(struct shmque), 0644 | IPC_CREAT);
			// Delete any message queue previously created
			msgId = msgget(KEY + i, 0666 | IPC_CREAT);
			msgctl(msgId, IPC_RMID, NULL);
			// Create message queue for the officer.
			msgId = msgget(KEY + i, 0666 | IPC_CREAT);
			// Initialize queue count to 0.
			shmq = shmat(shmId, NULL, 0);
			if (shmq == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			shmq->queue_count = 0;
			// Update the queue type
			if (i < no_crossing_points_p)
			{
				shmq->queue_type = QUEUE_P;
			}
			else if (i < (no_crossing_points_p + no_crossing_points_j))
			{
				shmq->queue_type = QUEUE_J;
			}
			else
			{
				shmq->queue_type = QUEUE_F;
			}
			if (shmdt(shmq) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			// Officer child process has to run infinitely
			while (true)
			{
				ex = shmat(exitId, NULL, 0);
				if (ex == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				// Check the simulation end condition.
				if (ex->totalCount >= max_allowed ||
					ex->totalDenied >= max_denied ||
					ex->totalLeft >= max_left)
				{
					if (shmdt(ex) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
					fprintf(stderr, "Simulation Ends\n");
					exit(0);
				}
				if (shmdt(ex) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}
				// Check if the Hall limit is reached maximum.
				hall = shmat(shmHId, NULL, 0);
				if (hall == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				hallCount = hall->total;
				if (shmdt(hall) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}
				// Wait untill hall limit reduces.
				if ((hallCount <= hall_min) ||
					(hallCount < hall_max && hallCount > hall_min))
				{
					// Waiting for messages.
					msgrcv(msgId, &msg, sizeof(msg), 0, 0);
					shmpass = shmat(msg.shmId, NULL, 0);
					if (shmpass == (void *)-1)
					{
						fprintf(stderr, "Shared memory attach error !\n");
					}
					else
					{
						// Check if the passenger has left the queue.
						if (shmpass->isTimedOut)
						{
							shmq = shmat(shmId, NULL, 0);
							if (shmq == (void *)-1)
							{
								fprintf(stderr, "Shared memory attach error !\n");
							}
							// If passenger has left the queue reduce the queue count.
							shmq->queue_count--;
							if (shmdt(shmq) == -1)
							{
								fprintf(stderr, "shmdt\n");
							}
							if (shmdt(shmpass) == -1)
							{
								fprintf(stderr, "shmdt\n");
							}
							shmctl(msg.shmId, IPC_RMID, NULL);
						}
						else
						{
							if (shmdt(shmpass) == -1)
							{
								fprintf(stderr, "shmdt\n");
							}
							// Get a random processing time for the officer.
							timeout = get_random_value(officer_timeout_lower, officer_timeout_higher);
							usleep(timeout * 1000000);
							// Check if passport is valid
							if (msg.isPassportValid)
							{
								shmpass = shmat(msg.shmId, NULL, 0);
								if (shmpass == (void *)-1)
								{
									fprintf(stderr, "Shared memory attach error !\n");
								}
								// Update passenger processing status.
								shmpass->isProcessed = PROCESSED;
								if (shmdt(shmpass) == -1)
								{
									fprintf(stderr, "shmdt\n");
								}
								// Even if the passenger is approved but if Hall is full,
								// wait until enough space is available in Hall.
								while (true)
								{
									hall = shmat(shmHId, NULL, 0);
									if (hall == (void *)-1)
									{
										fprintf(stderr, "Shared memory attach error !\n");
									}
									// Check current hall count is less than hall threshold value
									if (hall->total < hall_max)
									{
										// Increase hall count
										hall->total++;
										ex = shmat(exitId, NULL, 0);
										if (ex == (void *)-1)
										{
											fprintf(stderr, "Shared memory attach error !\n");
										}
										// Increment total passengers approved.
										ex->totalCount++;
										if (shmdt(ex) == -1)
										{
											fprintf(stderr, "shmdt\n");
										}
										shmq = shmat(shmId, NULL, 0);
										if (shmq == (void *)-1)
										{
											fprintf(stderr, "Shared memory attach error !\n");
										}
										// Check queue type and increment respective approved count
										if (shmq->queue_type == QUEUE_P)
											hall->totalApprovedP++;
										else if (shmq->queue_type == QUEUE_J)
											hall->totalApprovedJ++;
										else
											hall->totalApprovedF++;
										if (shmdt(shmq) == -1)
										{
											fprintf(stderr, "shmdt\n");
										}
										if (shmdt(hall) == -1)
										{
											fprintf(stderr, "shmdt\n");
										}
										break;
									}
									if (shmdt(hall) == -1)
									{
										fprintf(stderr, "shmdt\n");
									}
									shmq = shmat(shmId, NULL, 0);
									if (shmq == (void *)-1)
									{
										fprintf(stderr, "Shared memory attach error !\n");
									}
									// Reduce queue count.
									shmq->queue_count--;
									if (shmdt(shmq) == -1)
									{
										fprintf(stderr, "shmdt\n");
									}
									usleep(500000);
								}
							}
							else
							{
								shmpass = shmat(msg.shmId, NULL, 0);
								if (shmpass == (void *)-1)
								{
									fprintf(stderr, "Shared memory attach error !\n");
								}
								// Passenger is denied crossing border.
								shmpass->isProcessed = DENIED;
								if (shmdt(shmpass) == -1)
								{
									fprintf(stderr, "shmdt\n");
								}

								shmq = shmat(shmId, NULL, 0);
								if (shmq == (void *)-1)
								{
									fprintf(stderr, "Shared memory attach error !\n");
								}
								// Reduce queue count. Because the passenger is either approved or denied and left the queue.
								shmq->queue_count--;
								if (shmdt(shmq) == -1)
								{
									fprintf(stderr, "shmdt\n");
								}
							}
						}
					}
				}
				usleep(2000000);
			}
		}
	}
}

/*
  Function name : get_random_value
  Args : int, int
  Return value : int

  Description : Function to generate a random integer to introduce delay.
 */
int get_random_value(int lower, int upper)
{
	srand(time(0));
	return (rand() % (upper - lower + 1)) + lower;
}

/*
  Function name : create_buses
  Args : None
  Returns : None

  Description : Function to create processes representing buses.
 */
void create_buses()
{
	int pid;
	int exitId, shmHId, busId;
	struct exitDb *ex;
	struct hallDb *hall;
	struct busDb *bus;

	exitId = shmget(EXIT_KEY, sizeof(struct exitDb), 0644 | IPC_CREAT);
	shmHId = shmget(HALL_KEY, sizeof(struct hallDb), 0644 | IPC_CREAT);

	// Loop to create process representing buses based on the total bus count.
	for (int i = 0; i < no_buses; i++)
	{
		pid = fork();
		if (pid == 0)
		{
			// Create shared memory for buses and intialize variables.
			busId = shmget(BUS_KEY + i, sizeof(struct busDb), 0644 | IPC_CREAT);
			bus = shmat(busId, NULL, 0);
			if (bus == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			bus->busId = i;
			bus->busStatus = BUS_LOADING;
			bus->count = 0;
			if (shmdt(bus) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			// Bus processes runs infinitely.
			while (true)
			{
				// Check the simultion end condition.
				ex = shmat(exitId, NULL, 0);
				if (ex == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				if (ex->totalCount >= max_allowed ||
					ex->totalDenied >= max_denied ||
					ex->totalLeft >= max_left)
				{
					if (shmdt(ex) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
					fprintf(stderr, "Simulation Ends\n");
					exit(0);
				}
				if (shmdt(ex) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}

				bus = shmat(busId, NULL, 0);
				if (bus == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				// Check if bus count is less than max bus capacity.
				if (bus->count < bus_capacity)
				{
					bus->busStatus = BUS_LOADING;
					hall = shmat(shmHId, NULL, 0);
					if (hall == (void *)-1)
					{
						fprintf(stderr, "Shared memory attach error !\n");
					}
					// Load passenger one by one.
					if (hall->total != 0)
					{
						bus->count++;
						hall->total--;
					}
					// Check if bus has reached maximum capacity.
					if (bus->count == bus_capacity)
					{
						bus->busStatus = BUS_TRIP; // Update bus status
					}
					if (shmdt(hall) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
					if (shmdt(bus) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
				}
				else
				{
					if (shmdt(bus) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
					// Time take by bus to complete the trip.
					usleep(bus_trip_delay * 1000000);
					bus = shmat(busId, NULL, 0);
					if (bus == (void *)-1)
					{
						fprintf(stderr, "Shared memory attach error !\n");
					}
					// Passenger count in bus becomes 0 after the trip.
					bus->count = 0;
					if (shmdt(bus) == -1)
					{
						fprintf(stderr, "shmdt\n");
					}
				}
				// Passenger loading delay.
				usleep(1000000);
			}
		}
	}
}

/*
  Function name : print_data
  Args : None
  Returns : None

  Description : Function to print information in the console.

*/
void print_data()
{

	int pid;
	int exitId, shmId, shmHId, busId, shmDId;
	struct exitDb *ex;
	struct hallDb *hall;
	struct busDb *bus;
	struct shmque *shmq;
	struct deniedDb *denied;

	exitId = shmget(EXIT_KEY, sizeof(struct exitDb), 0644 | IPC_CREAT);
	shmHId = shmget(HALL_KEY, sizeof(struct hallDb), 0644 | IPC_CREAT);
	shmDId = shmget(DENIED_KEY, sizeof(struct deniedDb), 0644 | IPC_CREAT);

	pid = fork();
	if (pid == 0)
	{
		while (true)
		{
			system("clear");
			printf("Queue 1 to %d : Palestinians\n", no_crossing_points_p);
			printf("Queue %d to %d : Jordanians\n", no_crossing_points_p + 1, no_crossing_points_p + no_crossing_points_j);
			printf("Queue %d to %d : Foreigners\n", no_crossing_points_p + no_crossing_points_j + 1, no_officers);
			printf("===========================================================================================\n\n");

			for (int i = 0; i < no_officers; i++)
			{
				shmId = shmget(KEY + i, sizeof(struct shmque), 0644 | IPC_CREAT);
				shmq = shmat(shmId, NULL, 0);
				if (shmq == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				printf("Queue - %d : Number of Passengers in queue = %d\n", i + 1, shmq->queue_count);
				if (shmdt(shmq) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}
			}
			printf("===========================================================================================\n\n");
			hall = shmat(shmHId, NULL, 0);
			if (hall == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			printf("Total passengers waiting in hall  = %d\n", hall->total);
			printf("Total Palestinians allowed today  = %d\n", hall->totalApprovedP);
			printf("Total Jordanians allowed today    = %d\n", hall->totalApprovedJ);
			printf("Total Foreigners allowed today    = %d\n", hall->totalApprovedF);
			printf("===========================================================================================\n\n");
			if (shmdt(hall) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			denied = shmat(shmDId, NULL, 0);
			if (denied == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			printf("Total Palestinians denied today    = %d\n", denied->totalDeniedP);
			printf("Total Jordanians denied today      = %d\n", denied->totalDeniedJ);
			printf("Total Foreigners denied today      = %d\n", denied->totalDeniedF);
			printf("Total Palestinians left Queue      = %d\n", denied->totalLeftP);
			printf("Total Jordanians left Queue        = %d\n", denied->totalLeftJ);
			printf("Total Foreigners left Queue        = %d\n", denied->totalLeftF);
			printf("===========================================================================================\n\n");

			if (shmdt(denied) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}

			for (int i = 0; i < no_buses; i++)
			{
				busId = shmget(BUS_KEY + i, sizeof(struct busDb), 0644 | IPC_CREAT);
				bus = shmat(busId, NULL, 0);
				if (bus == (void *)-1)
				{
					fprintf(stderr, "Shared memory attach error !\n");
				}
				if (bus->busStatus == BUS_LOADING)
				{
					printf("Bus Number %d is Loading passengers ! Current count = %d\n", i + 1, bus->count);
				}
				else
				{
					printf("Bus Number %d is on Trip...\n", i + 1);
				}
				if (shmdt(bus) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}
			}
			printf("===========================================================================================\n\n");

			ex = shmat(exitId, NULL, 0);
			if (ex == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}
			printf("Total Passengers Allowed today     = %d\n", ex->totalCount);
			printf("Total Passengers Denied today      = %d\n", ex->totalDenied);
			printf("Total Passengers Left today        = %d\n", ex->totalLeft);
			printf("===========================================================================================\n\n");

			if (shmdt(ex) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			ex = shmat(exitId, NULL, 0);
			if (ex == (void *)-1)
			{
				fprintf(stderr, "Shared memory attach error !\n");
			}

			// Check simulation end condition.
			if (ex->totalCount >= max_allowed ||
				ex->totalDenied >= max_denied ||
				ex->totalLeft >= max_left)
			{
				if (shmdt(ex) == -1)
				{
					fprintf(stderr, "shmdt\n");
				}
				fprintf(stderr, "Simulation Ends\n");
				exit(0);
			}
			if (shmdt(ex) == -1)
			{
				fprintf(stderr, "shmdt\n");
			}
			usleep(1000000);
		}
	}
}
