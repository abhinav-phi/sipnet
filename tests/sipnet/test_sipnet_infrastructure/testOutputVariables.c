#include <stdio.h>
#include <string.h>

#include "common/logging.h"
#include "sipnet/outputVariables.h"

static int failures;

static void expect(int condition, const char *message) {
  if (!condition) {
    logTest("FAIL: %s\n", message);
    failures++;
  }
}

int main(void) {
  logTest("Starting test testOutputVariables\n");

  expect(setupOutputVariables("all") == 0, "all should parse");
  expect(outputVariablesAreAll() == 1, "all should select every variable");

  expect(setupOutputVariables("gpp,nee,soilWater") == 0, "a list should parse");
  expect(outputVariablesAreAll() == 0, "a list should not select all");
  expect(getOutputVariableCount() == 3, "a list should select three variables");
  expect(strcmp(getOutputVariableName(0), "gpp") == 0, "first is gpp");
  expect(strcmp(getOutputVariableName(1), "nee") == 0, "second is nee");
  expect(strcmp(getOutputVariableName(2), "soilWater") == 0,
         "third is soilWater");

  expect(setupOutputVariables(" nee , gpp , nee ") == 0,
         "spaces and duplicates should parse");
  expect(getOutputVariableCount() == 2, "duplicates should be dropped");
  expect(strcmp(getOutputVariableName(0), "nee") == 0, "first is nee");
  expect(strcmp(getOutputVariableName(1), "gpp") == 0, "second is gpp");

  expect(setupOutputVariables("") != 0, "an empty list is rejected");
  expect(setupOutputVariables("gpp,,nee") != 0, "an empty name is rejected");
  expect(setupOutputVariables("gpp,bogus") != 0, "an unknown name is rejected");
  expect(setupOutputVariables("all,gpp") != 0,
         "all may not be combined with other names");

  if (failures > 0) {
    logTest("FAILED testOutputVariables with %d failures\n", failures);
    return 1;
  }
  logTest("PASSED testOutputVariables\n");
  return 0;
}
