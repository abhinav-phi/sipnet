// Selection and writing of the variables in sipnet.out

#ifndef OUTPUT_VARIABLES_H
#define OUTPUT_VARIABLES_H

#include <stdio.h>

/* Select the variables to write to sipnet.out from a comma separated list.
 * "all" (the default) selects every variable. year, day and time are always
 * written first. Returns 0 on success. On a malformed, empty or unknown list
 * logs the problem with the list of valid names and returns 1. */
int setupOutputVariables(const char *spec);

/* True when every variable is selected (the default). */
int outputVariablesAreAll(void);

/* Number of selected variables, not counting year, day and time. */
int getOutputVariableCount(void);

/* Name of the i-th selected variable, in output order. */
const char *getOutputVariableName(int index);

/* Write the selected variable names, separated by spaces, without newline. */
void writeOutputVariableHeader(FILE *out);

/* Write the selected variable values, separated by spaces, without newline. */
void writeOutputVariableValues(FILE *out);

#endif
