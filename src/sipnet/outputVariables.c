#include "sipnet/outputVariables.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "common/logging.h"
#include "sipnet/state.h"

typedef struct OutputVariable {
  const char *name;
  const double *value;
  double (*compute)(void);
  const char *format;
} OutputVariable;

static double computeLai(void) {
  return envi.plantLeafC / params.leafCSpWt;
}

// Keep this table and the output formats in sync with the sipnet.out schema.
static const OutputVariable outputVariables[] = {
    {"plantWoodC", NULL, getTotalWoodC, " %10.2f"},
    {"plantLeafC", &envi.plantLeafC, NULL, " %10.2f"},
    {"woodCreation", &trackers.woodCreation, NULL, " %12.2f"},
    {"soil", &envi.soilC, NULL, " %8.2f"},
    {"coarseRootC", &envi.coarseRootC, NULL, " %11.2f"},
    {"fineRootC", &envi.fineRootC, NULL, " %9.2f"},
    {"litter", &envi.litterC, NULL, " %8.2f"},
    {"soilWater", &envi.soilWater, NULL, " %10.3f"},
    {"soilWetnessFrac", &trackers.soilWetnessFrac, NULL, " %15.3f"},
    {"snow", &envi.snow, NULL, " %8.2f"},
    {"npp", &trackers.npp, NULL, " %8.3f"},
    {"nee", &trackers.nee, NULL, " %8.3f"},
    {"cumNEE", &trackers.totNee, NULL, " %8.3f"},
    {"gpp", &trackers.gpp, NULL, " %8.3f"},
    {"rAboveground", &trackers.rAboveground, NULL, " %12.3f"},
    {"rSoil", &trackers.rSoil, NULL, " %8.3f"},
    {"rRoot", &trackers.rRoot, NULL, " %8.3f"},
    {"ra", &trackers.ra, NULL, " %8.3f"},
    {"rh", &trackers.rh, NULL, " %8.3f"},
    {"rtot", &trackers.rtot, NULL, " %8.3f"},
    {"evapotranspiration", &trackers.evapotranspiration, NULL, " %18.8f"},
    {"fluxestranspiration", &fluxes.transpiration, NULL, " %19.4f"},
    {"minN", &envi.minN, NULL, " %8.4f"},
    {"soilOrgN", &envi.soilOrgN, NULL, " %9.4f"},
    {"litterN", &envi.litterN, NULL, " %10.4f"},
    {"plantStorageN", &envi.plantStorageN, NULL, " %14.4f"},
    {"n2o", &trackers.n2o, NULL, " %9.6f"},
    {"nLeaching", &trackers.nLeaching, NULL, " %9.4f"},
    {"nFixation", &trackers.nFixation, NULL, " %10.4f"},
    {"nUptake", &trackers.nUptake, NULL, " %8.4f"},
    {"ch4", &trackers.methane, NULL, " %8.4f"},
    {"nppStorage", &envi.plantCAccountingDelta, NULL, " %12.4f"},
    {"lai", NULL, computeLai, " %10.4f"},
};

#define NUM_OUTPUT_VARIABLES                                                   \
  ((int)(sizeof(outputVariables) / sizeof(outputVariables[0])))

static int selected[NUM_OUTPUT_VARIABLES];
static int selectedCount;
static int allSelected;

static const OutputVariable *findOutputVariable(const char *name) {
  int i;
  for (i = 0; i < NUM_OUTPUT_VARIABLES; i++) {
    if (strcmp(outputVariables[i].name, name) == 0) {
      return &outputVariables[i];
    }
  }
  return NULL;
}

static int variableIndex(const OutputVariable *variable) {
  return (int)(variable - outputVariables);
}

static void logValidOutputVariables(void) {
  char buffer[1024] = "";
  size_t used = 0;
  int i;
  for (i = 0; i < NUM_OUTPUT_VARIABLES; i++) {
    size_t nameLength = strlen(outputVariables[i].name);
    size_t needed = nameLength + (i == 0 ? 0 : 2) + 1;
    if (used + needed > sizeof(buffer)) {
      break;
    }
    if (i > 0) {
      strcat(buffer, ", ");
      used += 2;
    }
    strcat(buffer, outputVariables[i].name);
    used += nameLength;
  }
  logError("valid output variables are: %s\n", buffer);
}

static char *trimToken(char *token) {
  char *end;
  while (isspace((unsigned char)*token)) {
    token++;
  }
  end = token + strlen(token);
  while (end > token && isspace((unsigned char)end[-1])) {
    end--;
  }
  *end = '\0';
  return token;
}

int setupOutputVariables(const char *spec) {
  char *copy;
  char *trimmed;
  char *start;
  int tokenCount;
  int i;

  selectedCount = 0;
  allSelected = 0;

  if (spec == NULL) {
    logError("outvars is empty; use \"all\" or a comma separated list of "
             "variable names\n");
    logValidOutputVariables();
    return 1;
  }

  copy = (char *)malloc(strlen(spec) + 1);
  if (copy == NULL) {
    logError("out of memory while reading outvars\n");
    return 1;
  }
  strcpy(copy, spec);

  trimmed = trimToken(copy);
  if (strcmp(trimmed, "all") == 0) {
    allSelected = 1;
    free(copy);
    return 0;
  }

  tokenCount = 0;
  start = trimmed;
  while (start != NULL) {
    char *comma = strchr(start, ',');
    char *token;
    const OutputVariable *variable;
    int index;
    int duplicate;

    if (comma != NULL) {
      *comma = '\0';
    }
    token = trimToken(start);
    tokenCount++;
    if (*token == '\0') {
      logError("outvars contains an empty variable name\n");
      logValidOutputVariables();
      free(copy);
      return 1;
    }
    if (strcmp(token, "all") == 0) {
      logError("outvars \"all\" may not be combined with other variables\n");
      free(copy);
      return 1;
    }
    variable = findOutputVariable(token);
    if (variable == NULL) {
      logError("unknown output variable '%s'\n", token);
      logValidOutputVariables();
      free(copy);
      return 1;
    }
    index = variableIndex(variable);
    duplicate = 0;
    for (i = 0; i < selectedCount; i++) {
      if (selected[i] == index) {
        duplicate = 1;
        break;
      }
    }
    if (!duplicate) {
      selected[selectedCount++] = index;
    }
    start = comma != NULL ? comma + 1 : NULL;
  }
  free(copy);

  if (tokenCount == 0 || selectedCount == 0) {
    logError("outvars is empty; use \"all\" or a comma separated list of "
             "variable names\n");
    logValidOutputVariables();
    return 1;
  }
  return 0;
}

int outputVariablesAreAll(void) { return allSelected; }

int getOutputVariableCount(void) { return selectedCount; }

const char *getOutputVariableName(int index) {
  return outputVariables[selected[index]].name;
}

void writeOutputVariableHeader(FILE *out) {
  int i;
  for (i = 0; i < selectedCount; i++) {
    fprintf(out, " %s", outputVariables[selected[i]].name);
  }
}

void writeOutputVariableValues(FILE *out) {
  int i;
  for (i = 0; i < selectedCount; i++) {
    const OutputVariable *variable = &outputVariables[selected[i]];
    double value =
        variable->value != NULL ? *variable->value : variable->compute();
    fprintf(out, variable->format, value);
  }
}
