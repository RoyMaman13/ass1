

#ifndef COMMANDS_H_
#define COMMANDS_H_

#include<iostream>
#include <string.h>

#include <fstream>
#include <vector>
#include <iomanip>
#include "HybridAnomalyDetector.h"

using namespace std;

struct reportInterval {
    string description;
    long start;
    long end;
};

class states {
public:
    float threshold;
    vector<AnomalyReport> reports;
    int rowsNum;

    states() {
        this->rowsNum = 0;
        this->threshold = 0.9;
    }

    void setThreshold(float f) {
        this->threshold = f;
    }
};

class DefaultIO {
public:
    virtual string read() = 0;

    virtual void write(string text) = 0;

    virtual void write(float f) = 0;

    virtual void read(float *f) = 0;

    virtual ~DefaultIO() {}

    // you may add additional methods here
};

// you may add here helper classes


// you may edit this class
class Command {
protected:
    DefaultIO *dio;
public:
    string description;

    Command(DefaultIO *dio, string description) : dio(dio), description(description) {}

    virtual void execute(states *state) = 0;

    virtual ~Command() {}
};

// implement here your command classes
class uploadCsvCommand : public Command {
public:
    uploadCsvCommand(DefaultIO *dio) : Command(dio, "upload a time series csv file") {}

    virtual void execute(states *state) {
        dio->write("Please upload your local train CSV file.\n");
        ofstream trainOut("anomalyTrain.txt");
        string s = dio->read();
        while (s != "done") {
            if (trainOut.is_open())
                trainOut << s << endl;
            s = dio->read();
        }
        trainOut.close();
        dio->write("Upload complete.\n");

        dio->write("Please upload your local test CSV file.\n");
        ofstream textOut("anomalyText.txt");
        s = dio->read();
        while (s != "done") {
            if (textOut.is_open())
                textOut << s << endl;
            state->rowsNum++;
            s = dio->read();
        }
        state->rowsNum--;
        textOut.close();
        dio->write("Upload complete.\n");
    }
};

class algorithmSettings : public Command {
public:
    algorithmSettings(DefaultIO *dio) : Command(dio, "algorithm settings") {}

    virtual void execute(states *state) {
        float newThreshold;
        while (true) {
            dio->write("The current correlation threshold is ");
            dio->write(to_string(state->threshold));
            dio->write("\n");
            dio->write("Type a new threshold\n");
            newThreshold = stof(dio->read());
            if (newThreshold >= 0 && newThreshold <= 1) {
                state->threshold = newThreshold;
                return;
            }
            dio->write("please choose a value between 0 and 1.\n");
        }
    }
};

class detectAnomalies : public Command {
public:
    detectAnomalies(DefaultIO *dio) : Command(dio, "detect anomalies") {}

    virtual void execute(states *state) {
        HybridAnomalyDetector hybridAnomalyDetector;
        TimeSeries learnTs("anomalyTrain.txt");
        hybridAnomalyDetector.setThreshold(state->threshold);
        hybridAnomalyDetector.learnNormal(learnTs);
        TimeSeries testTs("anomalyText.txt");
        state->reports = hybridAnomalyDetector.detect(testTs);
        dio->write("anomaly detection complete.\n");
    }
};

class displayResults : public Command {
public:
    displayResults(DefaultIO *dio) : Command(dio, "display results") {}

    virtual void execute(states *state) {
        for (AnomalyReport anomalyReport: state->reports) {
            dio->write(anomalyReport.timeStep);
            dio->write("\t");
            dio->write(anomalyReport.description);
            dio->write("\n");
        }
        dio->write("Done.\n");
    }
};

class uploadAnomaliesAnalyzeResults : public Command {
public:
    uploadAnomaliesAnalyzeResults(DefaultIO *dio) : Command(dio, "upload anomalies and analyze results") {}

    virtual void execute(states *state) {
        vector<reportInterval> intervalVec;
        for (AnomalyReport anomalyReport: state->reports) {
            if (intervalVec.empty())
                intervalVec.push_back(
                        reportInterval{anomalyReport.description, anomalyReport.timeStep, anomalyReport.timeStep});
            else if ((anomalyReport.description.compare(intervalVec.at(intervalVec.size() - 1).description) == 0) &&
                     (anomalyReport.timeStep == (intervalVec.at(intervalVec.size() - 1).end + 1)))
                intervalVec.at(intervalVec.size() - 1).end++;
            else
                intervalVec.push_back(
                        reportInterval{anomalyReport.description, anomalyReport.timeStep, anomalyReport.timeStep});
        }

        dio->write("Please upload your local anomalies file.\n");
        string s = dio->read();
        vector<reportInterval> intervalResVec;
        while (s != "done") {
            int index = s.find(',');
            intervalResVec.push_back(
                    reportInterval{"", stol(s.substr(0, index)), stol(s.substr(index + 1, s.size()))});
            s = dio->read();
        }
        int P = intervalResVec.size();
        int sum = 0;
        for (int i = 0; i < intervalResVec.size(); i++) {
            sum += intervalResVec.at(i).end - intervalResVec.at(i).start + 1;
        }
        int N = state->rowsNum - sum;
        int FP = 0;
        int TP = 0;
        for (reportInterval report: intervalVec) {
            bool flag = false;
            for (reportInterval anomaly: intervalResVec) {
                if ((report.start <= anomaly.start && report.end >= anomaly.start) ||
                    (report.start <= anomaly.end && report.end >= anomaly.end) ||
                    (report.start >= anomaly.start && report.end <= anomaly.end)) {
                    TP++;
                    flag = true;
                }
            }
            if (flag == false)
                FP++;
        }
        setprecision(3);
        dio->write("Upload complete.\n");
        dio->write("True Positive Rate: ");
        int tpr = (int) (1000.0 * TP / P);
        int fpr = (int) (1000.0 * FP / N);
        dio->write((float) (tpr / 1000.00));
        dio->write("\n");
        dio->write("False Positive Rate: ");
        dio->write((float) (fpr / 1000.00));
        dio->write("\n");
    }
};

class exitMenu : public Command {
public:
    exitMenu(DefaultIO *dio) : Command(dio, "exit") {}

    virtual void execute(states *state) {
        dio->write(description);
    }
};


#endif /* COMMANDS_H_ */
