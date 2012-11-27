/* ------------------------------------------------------------------------- 
 * main.c
 * Implements simulation of a single FMU instance 
 * that implements the "FMI for Co-Simulation 1.0" interface.
 * Command syntax: see printHelp()
 * Simulates the given FMU from t = 0 .. tEnd with fixed step size h and 
 * writes the computed solution to file 'result.csv'.
 * The CSV file (comma-separated values) may e.g. be plotted using 
 * OpenOffice Calc or Microsoft Excel. 
 * This progamm demonstrates basic use of an FMU.
 * Real applications may use advanced master algorithms to cosimulate 
 * many FMUs, limit the numerical error using error estimation
 * and back-stepping, provide graphical plotting utilities, debug support, 
 * and user control of parameter and start values, or perform a clean
 * error handling (e.g. call freeSlaveInstance when a call to the fmu
 * returns with error). All this is missing here.
 *
 * Revision history
 *  22.08.2011 initial version released in FMU SDK 1.0.2
 *     
 * Free libraries and tools used to implement this simulator:
 *  - header files from the FMU specification
 *  - eXpat 2.0.1 XML parser, see http://expat.sourceforge.net
 *  - 7z.exe 4.57 zip and unzip tool, see http://www.7-zip.org
 * Author: Jakob Mauss
 * Copyright 2011 QTronic GmbH. All rights reserved. 
 * -------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fmi_cs.h"
#include "sim_support.h"

FMU fmu; // the fmu to simulate

// simulate the given FMU using the forward euler method.
// time events are processed by reducing step size to exactly hit tNext.
// state events are checked and fired only at the end of an Euler step. 
// the simulator may therefore miss state events and fires state events typically too late.
static int simulate(FMU** fmus, char* fmuFileNames[], int N, int* connections, int M, double tEnd, double h, fmiBoolean loggingOn, char separator) {
    int i;
    double time;                        // Current time
    double tStart = 0;                  // Start time
    const char** guid;                  // Array of string GUIDs; global unique id of each fmu
    fmiComponent* c;                    // Array of FMU instances
    fmiStatus fmiFlag;                  // return code of the fmu functions
    fmiStatus status = fmiOK;           // return code of the fmu functions
    const char** fmuLocation;           // path to the fmu as URL, "file://C:\QTronic\sales"
    const char* mimeType = "application/x-fmu-sharedlibrary"; // denotes tool in case of tool coupling
    fmiReal timeout = 1000;             // wait period in milliseconds, 0 for unlimited wait period
    fmiBoolean visible = fmiFalse;      // no simulator user interface
    fmiBoolean interactive = fmiFalse;  // simulation run without user interaction
    fmiCallbackFunctions callbacks;     // called by the model during simulation
    ModelDescription** md;              // handle to the parsed XML file   
    int nSteps = 0;                     // Number of steps taken
    FILE** files;                       // result files
    char** fileNames;                   // Result file names


    // Allocate
    c =           (fmiComponent*)calloc(sizeof(fmiComponent),N);
    guid =        (const char**)calloc(sizeof(char*),N);
    fmuLocation = (const char**)calloc(sizeof(char*),N);
    md =          (ModelDescription**)calloc(sizeof(ModelDescription*),N);
    files =       (FILE**)calloc(sizeof(FILE*),N);
    fileNames =   (char**)calloc(sizeof(char*),N);

    // instantiate the fmu 
    callbacks.logger = fmuLogger;
    callbacks.allocateMemory = calloc;
    callbacks.freeMemory = free;
    callbacks.stepFinished = NULL; // fmiDoStep has to be carried out synchronously

    // Init all the FMUs
    for(i=0; i<N; i++){
        md[i] = (ModelDescription*)fmus[i]->modelDescription;


        guid[i] = getString(md[i], att_guid);
        c[i] = fmus[i]->instantiateSlave(getModelIdentifier(md[i]), guid[i], fmuLocation[i], mimeType, timeout, visible, interactive, callbacks, loggingOn);
        if (!c[i]) return error("could not instantiate model");

        // Generate out file name like this: "resultN.csv" where N is the FMU index
        fileNames[i] = calloc(sizeof(char),100);
        sprintf(fileNames[i],"result%d.csv",i);

        // open result file
        if (!(files[i] = fopen(fileNames[i], "w"))) {
            printf("could not write %s because:\n", fileNames[i]);
            printf("    %s\n", strerror(errno));
            return 0; // failure
        }
        
        // StopTimeDefined=fmiFalse means: ignore value of tEnd
        fmiFlag = fmus[i]->initializeSlave(c[i], tStart, fmiTrue, tEnd);
        if (fmiFlag > fmiWarning){
            printf("Could not initialize model %s",fmuFileNames[i]);
            return 0;
        }
        
        // output solution for time t0
        outputRow(fmus[i], c[i], tStart, files[i], separator, TRUE);  // output column names
        outputRow(fmus[i], c[i], tStart, files[i], separator, FALSE); // output values

    }

    // enter the simulation loop
    time = tStart;
    int k;
    int l;
    fmiReal r;
    fmiInteger ii;
    fmiBoolean b;
    fmiString s;
    fmiValueReference vrFrom;
    fmiValueReference vrTo;
    int found = 0;
    while (time < tEnd && status==fmiOK) {

        /*
        Basically do this to transfer values from output to input:
        fmiGetReal(s1, ..., 1, &y1);
        fmiGetReal(s2, ..., 1, &y2);
        fmiSetReal(s1, ..., 1, &y2);
        fmiSetReal(s2, ..., 1, &y1);
        */
        int ci;
        for(ci=0; ci<M; ci++){
            found = 0;
            int fmuFrom = connections[ci*4];
            vrFrom =  (fmiValueReference)connections[ci*4 + 1];
            int fmuTo =   connections[ci*4 + 2];
            vrTo =    (fmiValueReference)connections[ci*4 + 3];
            for (k=0; !found && fmus[fmuFrom]->modelDescription->modelVariables[k]; k++) {
                ScalarVariable* svFrom = fmus[fmuFrom]->modelDescription->modelVariables[k];
                if (getAlias(svFrom)!=enu_noAlias) continue;

                // Is this the correct one?
                if(vrFrom == getValueReference(svFrom)){

                    // Now find the input variable
                    for (l=0; !found && fmus[fmuTo]->modelDescription->modelVariables[l]; l++) {
                        ScalarVariable* svTo = fmus[fmuTo]->modelDescription->modelVariables[l];
                        if (getAlias(svTo)!=enu_noAlias) continue;

                        // Found the input and output. Check if they have equal types
                        if(svFrom->typeSpec->type == svFrom->typeSpec->type){

                            // Same types! Transfer...
                            switch (svFrom->typeSpec->type){
                                case elm_Real:
                                    fmus[fmuFrom]->getReal(c[fmuFrom], &vrFrom, 1, &r);
                                    fmus[fmuTo  ]->setReal(c[fmuTo  ], &vrTo,   1, &r);
                                    break;
                                case elm_Integer:
                                case elm_Enumeration:
                                    fmus[fmuFrom]->getInteger(c[fmuFrom], &vrFrom, 1, &ii);
                                    fmus[fmuTo  ]->setInteger(c[fmuTo  ], &vrTo,   1, &ii);
                                    break;
                                case elm_Boolean:
                                    fmus[fmuFrom]->getBoolean(c[fmuFrom], &vrFrom, 1, &b);
                                    fmus[fmuTo  ]->setBoolean(c[fmuTo  ], &vrTo,   1, &b);
                                    break;
                                case elm_String:
                                    fmus[fmuFrom]->getString(c[fmuFrom], &vrFrom, 1, &s);
                                    fmus[fmuTo  ]->setString(c[fmuTo  ], &vrTo,   1, &s);
                                    break;
                                default: 
                                    printf("Unrecognized file type\n");
                            }

                            found = 1;
                        } else {
                            printf("Connection between FMU %d (value ref %d) and %d (value ref %d) had incompatible data types!\n",fmuFrom,vrFrom,fmuTo,vrTo);
                        }
                    }
                }
            }
        }

        // Step all the FMUs
        for(i=0; i<N; i++){
            fmiFlag = fmus[i]->doStep(c[i], time, h, fmiTrue);
            if(fmiFlag != fmiOK){
                status = fmiFlag;
                printf("doStep() of model %s didn't return fmiOK! Exiting...",fmuFileNames[i]);
                return 0;
            }
        }

        // Advance time
        time += h;

        // Write to files
        for(i=0; i<N; i++){
            outputRow(fmus[i], c[i], time, files[i], separator, FALSE); // output values for this step
        }
        nSteps++;
    }
    
    // end simulation
    for(i=0; i<N; i++){
        fmiFlag = fmus[i]->terminateSlave(c[i]);
        fmus[i]->freeSlaveInstance(c[i]);
    }
  
    // print simulation summary 
    printf("Simulation from %g to %g terminated successful\n", tStart, tEnd);
    printf("  steps ............ %d\n", nSteps);
    printf("  fixed step size .. %g\n", h);
    for(i=0; i<N; i++){
        fclose(files[i]);
        printf("CSV file '%s' written\n", fileNames[i]);
    }

    return 1; // success
}

int main(int argc, char *argv[]) {
    int i;
    int N = 1;
    int *connections;
    char **fileNames;
    int M = 0;
    double tEnd = 1.0;
    double h = 0.1;
    int loggingOn = 1;
    char csv_separator = ',';
    parseArguments2(argc, argv, &N, &fileNames, &M, &connections, &tEnd, &h, &loggingOn, &csv_separator);

    // Allocate FMUs and load them
    FMU** fmus = (FMU**)calloc(sizeof(FMU*), N);
    for(i=0; i<N; i++){
        fmus[i] = (FMU*)calloc(sizeof(FMU),1);
        loadFMU2(fileNames[i],fmus[i]);
    }

    // run the simulation
    printf("FMU Simulator: run %d FMU(s) with %d connection(s) from t=0..%g with h=%g, loggingOn=%d, csv separator='%c'\n", 
            N, M, tEnd, h, loggingOn, csv_separator);
    simulate(fmus, fileNames, N, connections, M, tEnd, h, loggingOn, csv_separator);

    // release FMU
    for(i=0; i<N; i++){
#ifdef _MSC_VER
        FreeLibrary(fmus[i]->dllHandle);
#else
        dlclose(fmus[i]->dllHandle);
#endif
        freeElement(fmus[i]->modelDescription);
    }

    // Free vars
    for(i=0; i<N; i++){
        free(fmus[i]);
    }
    free(fmus);
    free(connections);

    return EXIT_SUCCESS;
}

