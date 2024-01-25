import numpy as np
import pandas as pd
import os
import random

path = "../data/"
events = pd.read_csv(path+"events.csv", sep=";").dropna().reset_index()
surveys = pd.read_csv(path+"surveys.csv", sep=";").dropna().reset_index()

topics = ["euthanasia", "fssocsec", "fswelfare", "jobguar", "marijuana", "toomucheqrights"]

#change format to datetime
for i in range(1,7):
    col = "completed_" + str(i)
    surveys[col] = pd.to_datetime(surveys[col])
    
events["EventTime"] = pd.to_datetime(events["EventTime"])

def change_state(sender,receiver):
    if sender == 0 and receiver == 0:
        return 0
    if sender == 0 and receiver == 1:
        return 2
    if sender == 0 and receiver == 2:
        return 0
    
    if sender == 1 and receiver == 0:
        return 2
    if sender == 1 and receiver == 1:
        return 1
    if sender == 1 and receiver == 2:
        return 1
    
    if sender == 2:
        opinion = random.choice([0,1])
        return change_state(opinion,receiver)

def run_NG(topic):

    # Create df with current state of students
    df = surveys[["egoid", topic + "_1"]].copy()
    df["current_opinion"] = df[topic + "_1"]
    df["current_survey"] = 2

    # Create df with results
    results = pd.DataFrame(columns=["StudentID", "SurveyDate", "SurveyNr", "Question", "OpinionSurvey", "OpinionSimulation"])

    # Iterate through events
    for index, row in events.iterrows():
        receiver = row["ReceiverID"]
        sender = row["SenderID"]
        event_time = row["EventTime"]
        #print(index)

        try:
            # Update current reciever state 
            sender_state = df.loc[df["egoid"] == sender, "current_opinion"].iloc[0]
            receiver_state = df.loc[df["egoid"] == receiver, "current_opinion"].iloc[0]
            updated_receiver_state = change_state(sender_state,receiver_state)
            df.loc[df["egoid"] == receiver, "current_state"] = updated_receiver_state
            
            
            # Compare event time with survey time and update results
            survey_num = df.loc[df["egoid"] == receiver, "current_survey"].iloc[0]
            survey_date = surveys.loc[surveys["egoid"] == receiver, "completed_" + str(survey_num)].iloc[0]
            survey_opinion = surveys.loc[surveys["egoid"] == receiver, topic + "_" + str(survey_num)].iloc[0]
            #print(event_time, survey_date,survey_num, sender, receiver,sender_state,receiver_state,updated_receiver_state)
            if survey_date<event_time:
                print("OK")
                df.loc[df["egoid"] == receiver, "current_survey"] += 1
                new_row_data = {
                    "StudentID": receiver,
                    "SurveyDate": survey_date,
                    "SurveyNr": survey_num,
                    
                    "Question": topic,
                    "OpinionSurvey": survey_opinion,
                    "OpinionSimulation": updated_receiver_state
                }
                results.loc[len(results)] = new_row_data

            #df.loc[df["egoid"] == receiver, "current_opinion"] = df.loc[df["egoid"] == sender, topic + "_1"].iloc[0]
        except Exception as e:
            #print(f"Error: {e}")
            pass

        #if index == 125000:
         #   break
            
    return results

evaluation = pd.DataFrame(columns=["topic", "accuracy"])

for topic in topics:
    for i in range(10):
        res = run_NG(topic)
        res["is_sim_survey"] = np.where(res['OpinionSimulation'] == res['OpinionSurvey'], 1, 0)
        accuracy = res.is_sim_survey.mean()
        evaluation.loc[len(evaluation)] = {"topic": topic,"accuracy": accuracy}
        res.to_csv("results_naming_game/naming_game_" + topic + str(i) + '.csv')  
        print(i, topic, accuracy)