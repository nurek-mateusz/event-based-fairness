// Notes:
// #1 when compiling using GCC, "-lm" compilation option is required
// #2 all the events have to be in chronological order, otherwise CogSNet calculations will be wrong
// #3 if some events' timestamps exceed the survey timestamp, they won't be read
// #4 the pathEvents and pathSurveyDates must contain the header and an empty line at the end
// #5 output file with CogSNet is not sorted by weights or anything else

#define _XOPEN_SOURCE
#define LEN(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// linear forgetting
float compute_weight_linear(int new_event, float weight_last_event, float time_difference, float lambda, float mu) {
	if(new_event == 1) {
		return(mu + (weight_last_event - time_difference * lambda) * (1 - mu));
	} else {
		return(weight_last_event - time_difference * lambda);
	}
}

// power forgetting
float compute_weight_power(int new_event, float weight_last_event, float time_difference, float lambda, float mu) {
	// We need to check whether the time_difference is greater or equal one,
	// since the power to sth smaller than one will result with heigher weight

	if(time_difference >= 1) {
		if(new_event == 1) {
			return(mu + (weight_last_event * pow(time_difference, -1 * lambda) * (1 - mu)));
		} else {
			return(weight_last_event * pow(time_difference, -1 * lambda));
		}
	} else {
			return(weight_last_event);
	}
}

// exponential forgetting
float compute_weight_exponential(int new_event, float weight_last_event, float time_difference, float lambda, float mu) {
	if(new_event == 1) {
		return(mu + (weight_last_event * exp(-1 * lambda * time_difference) * (1 - mu)));
	} else {
		return(weight_last_event * exp(-1 * lambda * time_difference));
	}
}

// computing the weight, invoked for every new event and at the end (surveys' dates)
float compute_weight(int time_to_compute, int time_last_event, char forgettingType[16], float weight_last_event, int new_event, float mu, float lambda, float theta, int units) {
	// Compute the time difference between events
	float time_difference = ((float)(time_to_compute - time_last_event) / (float)units);

	if (time_difference >= 0) {
		// First, we need to know how much time passed since last event
		// The time difference has to be zero or a positive value

		// Our new weigth of the edge
		float weight_new;

		if(strncmp(forgettingType, "linear", 6) == 0) {
			weight_new = compute_weight_linear(new_event, weight_last_event, time_difference, lambda, mu);
		} else if(strncmp(forgettingType, "power", 5) == 0) {
			weight_new = compute_weight_power(new_event, weight_last_event, time_difference, lambda, mu);
		} else if(strncmp(forgettingType, "exponential", 11) == 0) {
			weight_new = compute_weight_exponential(new_event, weight_last_event, time_difference, lambda, mu);
		}

		// We don't want in our opinion model a cutoff point
		//
		// if(weight_new <= theta)
		// 	// Is the weight lower or equal the threshold?
		// 	// If so, it will be zeroed
		// 	return(0);
		// else {
		// 	// This is the typical case, return the new weight
		// 	return(weight_new);
		// }
		return(weight_new);
	} else {
		// Time difference was less than zero
		return(-1);
	}

}

void save_cogsnet(int convertedStudentID, float opinionChangeWeightA, float opinionChangeWeightB, float surveyWeightA, float surveyWeightB, int surveyOpinion, int simOpinion, int surveyTime, int surveyNr, char* surveyQuestion, int realStudentIDs[], float mu, float theta, float lambda, char* forgettingType, float deltaThreshold, int units, char* pathResults, bool writeHeader) {
	FILE *filePointer;

	if (writeHeader) {
		filePointer = fopen(pathResults, "w");
	} else {
		filePointer = fopen(pathResults, "a");
	}

	//define single row for results
	char singleRow[1024];
	
	if(filePointer != NULL) {
		// writing header
		if (writeHeader) {
			fprintf(filePointer, "%s", "StudentID;SurveyDate;SurveyNr;Question;LastOpinionChangeWeightA;LastOpinionChangeWeightB;SurveyWeightA;SurveyWeightB;OpinionSim;OpinionSurvey;Mu;Theta;Lambda;ForgettingType;Delta;Units\n");
		}

		int studentID = realStudentIDs[convertedStudentID];

		snprintf(singleRow, sizeof singleRow, "%d;%d;%d;%s;%f;%f;%f;%f;%d;%d;%f;%f;%f;%s;%f;%d", studentID, surveyTime, surveyNr, surveyQuestion, opinionChangeWeightA, opinionChangeWeightB, surveyWeightA, surveyWeightB, simOpinion, surveyOpinion, mu, theta, lambda, forgettingType, deltaThreshold, units);
		
		fprintf(filePointer, "%s\n", singleRow);
	} else {
		printf("[ERROR] Saving CogSNet to %s: error opening file for writing\n\n", pathResults);
	}
	
	fclose(filePointer);
}

float getRandomNumber(float min, float max) {
    return min + ((float)rand() / RAND_MAX) * (max - min);
}

// function responsible for computing CogSNet and opinion weights
void compute_cogsnet(int events[][3], int numberOfEvents, int opinionsSurvey[][6], int opinionsSim[], int surveyDates[][6], int nextSurveyNr[], char* surveyQuestion, int realStudentIDs[], int numberOfStudents, float mu, float theta, float lambda, char* forgettingType, float deltaThreshold, int units, char* pathResults) {
	printf("[START] Computing CogSNet\n\n");

	printf("Parameters:\n");
	printf(" Number of events: %d\n", numberOfEvents);
	printf(" Survey question: %s\n", surveyQuestion);
	printf(" Number of students: %d\n", numberOfStudents);
	printf(" Mu: %f\n", mu);
	printf(" Theta: %f\n", theta);
	printf(" Lambda: %f\n", lambda);
	printf(" Forgetting: %s\n", forgettingType);
	printf(" Delta: %f\n", deltaThreshold);
	printf(" Units: %d\n", units);

	bool writeHeader = true;

	// we declare an array for storing last events of each student
	// [studentID][0] - last event of opinion A for a given studentID
	// [studentID][1] - last event of opinion B for a fiven studentID
	//
	// opinion A has value 0
	// opinion B has value 1
	// opinion AB has value 2
	long int recentEvents[numberOfStudents][2];

	// we declare an array for storing weights of each student
	// [studentID][0] - last weight of opinion A for a given studentID
	// [studentID][1] - last weight of opinion B for a given studentID
	float currentWeights[numberOfStudents][2];

	// We also store, for each student's opinion, the weights at the time of the last change.
	// If the weight is equal -1, it means there was no change in opinion between two surveys.
	// A special case occurs when the weight is different from -1, but the opinion in two consecutive surveys is the same. 
	// This means that between the surveys, the opinion changed at least twice, for example: A->B->A.
	float opinionChangeWeight[numberOfStudents][2];

	for (int i = 0; i < numberOfStudents; ++i) {
		opinionChangeWeight[i][0] = -1;
		opinionChangeWeight[i][1] = -1;
	}

	for(int i = 0; i < numberOfStudents; i++) {	
		// Initial weight values set on the basis of completed surveys.
		//
		// When someone supports opinion A or B, we assign a random value between 0.64 and 1 to one opinion, and a value between 0 and 0.34 to the other opinion.
		// In the case of someone having opinion AB, we assign random values between 0.34 and 0.66 to both A and B.
		//
		// In each of the above cases, we check if the randomly assigned values meet the alpha threshold.
		srand(time(NULL));
		if(opinionsSim[i] == 0) {
			do {
				currentWeights[i][0] = getRandomNumber(0.67, 1);
				currentWeights[i][1] = getRandomNumber(0, 0.33);
			} while (fabs(currentWeights[i][0] - currentWeights[i][1]) < deltaThreshold);
		} else if (opinionsSim[i] == 1) {
			do {
				currentWeights[i][1] = getRandomNumber(0.67, 1);
				currentWeights[i][0] = getRandomNumber(0, 0.33);
			} while (fabs(currentWeights[i][0] - currentWeights[i][1]) < deltaThreshold);
		} else {
			do {
				currentWeights[i][0] =  getRandomNumber(0.34, 0.66);
				currentWeights[i][1] = getRandomNumber(0.34, 0.66);
			} while (fabs(currentWeights[i][0] - currentWeights[i][1]) >= deltaThreshold);
		}

		// We need to set a reference time against which the initialized weights will decrease. 
		// For all students, we set the first event in the dataset.
		recentEvents[i][0] = events[0][2];
		recentEvents[i][1] = events[0][2];

		// Save the intitial opinions to the file.
		float weightA = currentWeights[i][0];
		float weightB = currentWeights[i][1];
		save_cogsnet(i, weightA, weightB, weightA, weightB, opinionsSurvey[i][0], opinionsSurvey[i][0], surveyDates[i][0], 1, surveyQuestion, realStudentIDs, mu, theta, lambda, forgettingType, deltaThreshold, units, pathResults, writeHeader);
		writeHeader = false;
	}

	printf("\n");
	printf("[START] Computing CogSNets at events' times\n");
	
	// number of events that have been used for building CogSNet
	int finalNumberOfEvents = 0;

	for(int i = 0; i < numberOfEvents; i++) {
		if (i % 10000 == 0) {
			time_t t;
			struct tm *current_time;
			time(&t);
			current_time = localtime(&t);
			printf("Iter %d/%d - time: %02d:%02d:%02d\n", i, numberOfEvents, current_time->tm_hour, current_time->tm_min, current_time->tm_sec);
		}

		// events have to be chronorogically ordered

        // new weight will be stored here
        double newWeight = 0;

		// get senderID and receiverID
		int sender = events[i][0];
		int receiver = events[i][1];
		int eventTime = events[i][2];

		// For a given receiver check if the survey time has been reached.
		// First we need to get the number of the next survey for a given student.
		// We need a loop for a special case when a student filled out surveys but didn't receive any messages for a long time.
		// e.g: surv 1 - surv 2 -> no message but student completed survey 2 
		//		surv 2 - surv 3 -> no message but student completed survey 3
		// 		surv 3 - surv 4 -> student completed survey 4 and he got messages
		// Thanks to the loop we won't miss saving the opinions and weights for survey 2
		int surveyNr = nextSurveyNr[receiver];
		for (int j = surveyNr; j < 6; ++j) {
			// Survey completion time
			int surveyTime = surveyDates[receiver][j];

			// If a survey isn't completed (surveyTIme = -1) then go to the next survey
			if (surveyTime == -1) {
				nextSurveyNr[receiver] = j + 1;
				continue;
			}

			// Check if the survey time has been reached
			if (eventTime > surveyTime) {
				// Get opinion weights at the survey time
				float surveyWeightA = compute_weight(surveyTime, recentEvents[receiver][0], forgettingType, currentWeights[receiver][0], 0, mu, lambda, theta, units);
				float surveyWeightB = compute_weight(surveyTime, recentEvents[receiver][1], forgettingType, currentWeights[receiver][1], 0, mu, lambda, theta, units);
				
				save_cogsnet(receiver, opinionChangeWeight[receiver][0], opinionChangeWeight[receiver][1], surveyWeightA, surveyWeightB, opinionsSurvey[receiver][j], opinionsSim[receiver], surveyTime, j+1, surveyQuestion, realStudentIDs, mu, theta, lambda, forgettingType, deltaThreshold, units, pathResults, writeHeader);
				
				// increment number of the next survey
				nextSurveyNr[receiver] = j + 1;

				// reset weights
				opinionChangeWeight[receiver][0] = -1;
				opinionChangeWeight[receiver][1] = -1;
			} else {
				// The survey time hasn't been reached yet, break the loop.
				break;
			}
		}

		// get the sender's and receiver's current opinion
		int senderOpinion = opinionsSim[sender];
		int receiverOpinion = opinionsSim[receiver];

		// Sender has AB opinion
		// In state AB, the sender randomly sends opinion A or B. The probability is proportional to the current weights.
		if(senderOpinion == 2) {
			// compute current weights
			double senderWeightA = compute_weight(eventTime, recentEvents[sender][0], forgettingType, currentWeights[sender][0], 0, mu, lambda, theta, units);
			double senderWeightB = compute_weight(eventTime, recentEvents[sender][1], forgettingType, currentWeights[sender][1], 0, mu, lambda, theta, units);

			// compute probability o sending A
			// probB = 1 - probA
			double probA = senderWeightA / (senderWeightA + senderWeightB);
			
			// rand [0;1]
			if(probA <= ((double)rand()) / RAND_MAX) {
				// send opinion A
				senderOpinion = 0;
			} else {
				// send opinion B
				senderOpinion = 1;
			}
		}
		
		// was there any event with this student before?
        // we check it by looking at weights array, since
        // meanwhile the weight could have dropped below theta
        if(currentWeights[receiver][senderOpinion] == 0) {
            // no events before, we set the weight to mu
            newWeight = mu;
        } else {
            // there was an event before
            newWeight = compute_weight(eventTime, recentEvents[receiver][senderOpinion], forgettingType, currentWeights[receiver][senderOpinion], 1, mu, lambda, theta, units);
        }

        // set the new last event time
        recentEvents[receiver][senderOpinion] = eventTime;
        
        // set the new weight
        currentWeights[receiver][senderOpinion] = newWeight;

		// compute the current weights for both opinions
		double weightA = compute_weight(eventTime, recentEvents[receiver][0], forgettingType, currentWeights[receiver][0], 0, mu, lambda, theta, units);
		double weightB = compute_weight(eventTime, recentEvents[receiver][1], forgettingType, currentWeights[receiver][1], 0, mu, lambda, theta, units);
		
		// check the difference in weights between opinions
		double delta = fabs(weightA - weightB);
		int newOpinion;
		if(delta < deltaThreshold) {
			// opinion AB
			newOpinion = 2;
		} else if(weightA > weightB) {
			// opinion A
			newOpinion = 0;
		} else {
			// opinion B
			newOpinion = 1;
		}

		// Check if there has been a change in opinion
		int oldOpinion = opinionsSim[receiver];
		if (newOpinion != oldOpinion) {
			opinionsSim[receiver] = newOpinion;

			// save weights at the time of changed
			opinionChangeWeight[receiver][0] = weightA;
			opinionChangeWeight[receiver][1] = weightB;
		}

        finalNumberOfEvents++;
	}

	// After processing all events, we check if there are any opinions left to save. 
	// Some students might have filled out surveys after the last event in the dataset.
	int lastEventTime = events[numberOfEvents-1][2];
	for (int studentID = 0; studentID < numberOfStudents; ++studentID) {
		int surveyNr = nextSurveyNr[studentID];
		for (int j = surveyNr; j < 6; ++j) {
			int surveyTime = surveyDates[studentID][j];

			if (surveyTime == -1) {
				continue;
			}

			if (surveyTime >= lastEventTime) {
				float surveyWeightA = compute_weight(surveyTime, recentEvents[studentID][0], forgettingType, currentWeights[studentID][0], 0, mu, lambda, theta, units);
				float surveyWeightB = compute_weight(surveyTime, recentEvents[studentID][1], forgettingType, currentWeights[studentID][1], 0, mu, lambda, theta, units);
				save_cogsnet(studentID, opinionChangeWeight[studentID][0], opinionChangeWeight[studentID][1], surveyWeightA, surveyWeightB, opinionsSurvey[studentID][j], opinionsSim[studentID], surveyTime, j+1, surveyQuestion, realStudentIDs, mu, theta, lambda, forgettingType, deltaThreshold, units, pathResults, writeHeader);
				printf("[DEBUG] surveyTime > lastEventTime: receiverID: %d, surveyNr: %d, surveyTime: %d, lastEventTime: %d\n", studentID, j+1, surveyTime, lastEventTime);
			}
		}
	}

	printf("[FINISH] Computing CogSNets at events' times (%d events considered)\n", finalNumberOfEvents);
}

// checks whether a given element exists in an array
int existingId(int x, int array[], int size){
	long int isFound = -1;

	int i = 0;

	while(isFound < 0 && i < size){
		if(array[i] == x){
			isFound = i;
			break;
		}

		i++;
	}
	return isFound;
}

// this function returns the element in the CSV organized as three-column one (x;y;timestamp)
// it is used to extract elements both from pathSurveyDates as from pathSurveyDates

int returnElementFromCSV(char eventLine[65536], int elementNumber, char delimiter[1], bool isNumber) {
	char *ptr;

	ptr = strtok(eventLine, delimiter);

	int thisLineElementNumber = 0;

	while(ptr != NULL) {
	  	if(thisLineElementNumber == elementNumber) {
	  		if(isNumber) {
	  			return atol(ptr);
	  		} else {
	  			struct tm tm;
				time_t epoch;

				if(strptime(ptr, "%Y-%m-%d %H:%M:%S", &tm) != NULL) {
					epoch = mktime(&tm);
					return(epoch);
				} else {
					return -1;
				}
			}
	  	}

		thisLineElementNumber++;

		ptr = strtok(NULL, delimiter);
	}
}

int readSurveys(char pathOpinions[255], int opinionsSurvey[][6], int* opinionsSim, int surveyDates[][6], int* nextSurveyNr, char* surveyQuestion, int* realStudentIDs, int numberOfStudents) {
    printf("[START] Reading opinions from %s\n", pathOpinions);

    int is_error = 0;

    // check if the file exists
    if(access(pathOpinions, F_OK) != -1) {
        // define the stream for events
        FILE* filePointer;
        char buffer[65536];
        char *line;
        char lineCopy[65536];
        char delimiter[] = ";";
        
        // open the pathOpinions with opinions as the stream
        filePointer = fopen(pathOpinions, "r");
            
        // check if there is no other problem with the file stream
        if(filePointer != NULL) {   
            // read the header
            line = fgets(buffer, sizeof(buffer), filePointer);

            int numberOfOpinions = 0;

            // now read the rest of the file until the condition won't be met
            while ((line = fgets(buffer, sizeof(buffer), filePointer)) != NULL) {
                int studentID = 0;
                int opinion = 0;

                // extract the first element (StudentID)
                strcpy(lineCopy, line);
                studentID = returnElementFromCSV(lineCopy, 0, delimiter, true);
                studentID = existingId(studentID, realStudentIDs, numberOfStudents);

				// extract survey times
				bool isNextSurvey = false;
				for (int i = 0; i < 6; ++i) {
					strcpy(lineCopy, line);
					int surveyTime = returnElementFromCSV(lineCopy, (6-i), delimiter, false);
					surveyDates[studentID][i] = surveyTime;

					if (isNextSurvey == false && i > 0 && surveyTime != -1) {
						nextSurveyNr[studentID] = i;
						isNextSurvey = true;
					}
				}
                
				// extract student's opinion
                int lastIndex = 0; 
				if (strcmp(surveyQuestion, "euthanasia") == 0) { 
					lastIndex = 12;
				} else if (strcmp(surveyQuestion, "fssocsec") == 0) {
					lastIndex = 18;
				} else if (strcmp(surveyQuestion, "fswelfare") == 0) {
					lastIndex = 24;
				} else if (strcmp(surveyQuestion, "jobguar") == 0) {
					lastIndex = 30;
				} else if (strcmp(surveyQuestion, "marijuana") == 0) {
					lastIndex = 36;
				} else if (strcmp(surveyQuestion, "toomucheqrights") == 0) {
					lastIndex = 42;
				} else {
					printf("[ERROR] Unknown survey question %s\n", surveyQuestion);
        			is_error = 1;
				}

				for (int i = lastIndex; i > (lastIndex - 6); --i) {
					strcpy(lineCopy, line);
					int opinion = returnElementFromCSV(lineCopy, i, delimiter, true);
					opinionsSurvey[studentID][lastIndex - i] = opinion;

					if (i == lastIndex) {
						opinionsSim[studentID] = opinion;
					}
				}

				numberOfOpinions++;
            }

            fclose(filePointer);

            printf("[FINISH] Reading opinions - %d student's opinion in total\n", numberOfOpinions);
            printf("\n");
        } else { 
            printf("[ERROR] Reading opinions from %s: error reading from filestream\n", pathOpinions);
            is_error = 1;
        }
    } else {
        printf("[ERROR] Reading opinions from %s: file not found\n", pathOpinions);
        is_error = 1;
    }

    return is_error;
}

int main(int argc, char** argv){
	// CogSNet parameters, see paper for details

	// Forgetting type
	//  linear - linear forgetting
	//  power - power forgetting
	//  exponential - exponential forgetting
	char forgettingType[16];
	strcpy(forgettingType, argv[1]);
	
	float mu = atof(argv[2]);
	float theta = atof(argv[3]);
	float lambda = atof(argv[4]);

	float delta = atof(argv[5]);

	// the units for forgetting, in seconds
	// usually, hours (3600)
	int units = atoi(argv[6]);

	// pathEvents with communication events
	char pathEvents[128];
	strcpy(pathEvents, argv[7]);

	// pathSurveyDates with information about survey times
	char pathSurveys[128];
	strcpy(pathSurveys, argv[8]);

	// pathCogSNet with information about where to save CogSNet
	char pathResults[255];
	strcpy(pathResults, argv[9]);

	char surveyQuestion[255];
	strcpy(surveyQuestion, argv[10]);

	// the new (converted) studentID of analysed student
	int convertedSurveyedStudentID = 0;

	// surveyDate as string, needed for exporting results
	char surveyDate[19];

	// the delimiter in the input/output files
	char delimiter[] = ";";

    //  the content of the pathEvents with events
    char buffer[65536];
    char *line;
    char lineCopy[65536];

    int numberOfEvents = 0;
    char chr;

    bool maxEventFound = false;

    printf("[START] Reading events from %s\n", pathEvents);
    
    // check if the file exists
    if(access(pathEvents, F_OK) != -1) {
        // define the stream for events
        FILE* filePointer;
        
        filePointer = fopen(pathEvents, "r");
            
        // check if there is no other problem with the file stream
        if(filePointer != NULL) {
            // read the header
            line = fgets(buffer, sizeof(buffer), filePointer);

            // now read the rest of the file - count events
            while ((line = fgets(buffer, sizeof(buffer), filePointer)) != NULL) {
                numberOfEvents++;
            }

            fclose(filePointer);

            if(numberOfEvents > 0) {
                // define an array for events
                // sender, receiver, timestamp
                int events[numberOfEvents][3];

                // now, reopen the pathEvents with events as the stream
                filePointer = fopen(pathEvents, "r");
                
                // skip first line, as it is a header
                line = fgets(buffer, sizeof(buffer), filePointer); 
                
                int senderID = 0;
                int receiverID = 0;
                int timestamp = 0;

                for(int eventNumber = 0; eventNumber < numberOfEvents; eventNumber++) {
                    // read the line
                    line = fgets(buffer, sizeof(buffer), filePointer);
                    
                    // extract the first element (sender's StudentID)
                    strcpy(lineCopy, line);
                    senderID = returnElementFromCSV(lineCopy, 0, delimiter, true);
                    
                    // extract the second element (receiver's StudentID)
                    strcpy(lineCopy, line);
                    receiverID = returnElementFromCSV(lineCopy, 1, delimiter, true);

                    // extract the second element (event timestamp)
                    strcpy(lineCopy, line);
                    timestamp = returnElementFromCSV(lineCopy, 2, delimiter, false);

                    // set the proper values of array
                    events[eventNumber][0] = senderID;
                    events[eventNumber][1] = receiverID;
                    events[eventNumber][2] = timestamp;
                }
                
                fclose(filePointer);

                printf("[FINISH] Reading events from %s - %d event(s) read\n", pathEvents, numberOfEvents);
                
                printf("\n");

                printf("[START] Converting StudentIDs\n");
                
                // the array holding real studentsIDs, sized numberOfEvents for the worst case
                int realStudentIDs[numberOfEvents];

                // actual number of students in the events files
                int numberOfStudents = 0;

                // ----- convert student ids -----
                for(int i = 0; i < numberOfEvents; i++) {
                    for(int j = 0; j < 2; j++) {
                        int realStudentID = events[i][j];
                        int convertedStudentID = existingId(realStudentID, realStudentIDs, numberOfStudents);

                        // do we already have this studentID in realStudentIDs?
                        if(convertedStudentID < 0) {
                            // no, we need to add it
                            realStudentIDs[numberOfStudents] = realStudentID;
                            events[i][j] = numberOfStudents;
                            numberOfStudents++;
                        } else {
                            events[i][j] = convertedStudentID;
                        }

                    }
                }

                printf("[FINISH] Converting StudentIDs - %d students in total\n", numberOfStudents);
                printf("\n");

				// student's opinions from the six surveys
                int opinionsSurvey[numberOfStudents][6];
				// current student's opinions from our simulation
				int opinionsSim[numberOfStudents];
				// the dates of completion for six surveys
				int surveyDates[numberOfStudents][6];
				// the number of the next survey for each student
				int nextSurvey[numberOfStudents];
                int is_error = readSurveys(pathSurveys, opinionsSurvey, opinionsSim, surveyDates, nextSurvey, surveyQuestion, realStudentIDs, numberOfStudents);

                if(is_error == 0) {
                    compute_cogsnet(events, numberOfEvents, opinionsSurvey, opinionsSim, surveyDates, nextSurvey, surveyQuestion, realStudentIDs, numberOfStudents, mu, theta, lambda, forgettingType, delta, units, pathResults);
                }
            } else {
                printf("[ERROR] Reading events from %s: no events to read\n", pathEvents);
            }
        } else {
            printf("[ERROR] Reading events from %s: error reading from filestream\n", pathEvents);
        } 
    } else {
            printf("[ERROR] Reading events from %s: file not found\n", pathEvents);
    }

	return 0;
}