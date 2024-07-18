# Event based fairness
This repository contains code and data samples for the CoDiNG opinion formation model which is based on CogSNet and Naming Game.
https://arxiv.org/abs/2406.19204

# Data
- `data/eventa.csv` – contains the communication history metadata from the NetSense study (`SenderID;ReceiverID;EventTime`)
- `data/surveys.csv` - ground truth - surveys completed by students who participated in the NetSense study
  - egoid - student ID
  - completed_x (where X is survey number 1-6) - date of the survey completion
  - Survey questions:
    - euthanasia_x - Should euthansia be legal
    - fssocsec_x - Do you think federal spending on social security should increase?
    - fswelfare_x - Do you think federal spending on welfare should increase?
    - jobguar_x - Do you think the government should see to it that every person has a job and a good standard of living?
    - marijuana_x - Should marijuana be legal?
    - toomucheqrights_x - Do you agree that we have gone too far in pushing equal rights in this country?
  
  Values encoding:
  - 0 - Disagree
  - 1 - Agree
  - 2- Not sure

# Scripts
To run the simulation execute the following command in the terminal: `./run.sh`

Parameters you may want to change in the run.sh script:

Line 16: ./opinion-dynamics exponential 0.3 0.2 0.00563145983483561 `DELTA` 3600 ../data/events.csv ../data/surveys.csv ../results/`FILE` `QUESTION`

- DELTA: value from the range (0,1)
- FILE: file with results
- QUESTION: one of the following values: euthanasia, fssocsec, fswelfare, jobguar, marijuana, toomucheqrights

# Results
The most important columns in files with results:
- StudentID
- SurveyNr - survey number (1-6)
- Question - survey question
- OpinionSim - opinion obtained from the model for the given StudentID
- OpinionSurvey - opinion from the surveys
