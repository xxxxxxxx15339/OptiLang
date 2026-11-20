function(add_cli_test name exit_code)
    cmake_parse_arguments(PARSE_ARGV 2 case "" "FILE;SOURCE;STATUS;OMIT" "TEXT;VALUES")

    if(DEFINED case_SOURCE)
        set(case_FILE "${CMAKE_CURRENT_BINARY_DIR}/cli_cases/${name}.opt")
        file(WRITE "${case_FILE}" "${case_SOURCE}")
    endif()

    # Pass list-valued expectations as single arguments to the checker.
    add_test(NAME ${name} COMMAND "${CMAKE_COMMAND}"
        "-DOPTILANG=$<TARGET_FILE:optilang>"
        "-DINPUT=${case_FILE}"
        "-DEXPECTED_EXIT=${exit_code}"
        "-DEXPECTED_TEXT=${case_TEXT}"
        "-DEXPECTED_VALUES=${case_VALUES}"
        "-DEXPECTED_STATUS=${case_STATUS}"
        "-DOMIT_VALUE=${case_OMIT}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_cli.cmake")
endfunction()

# Value expectations are name,lower,upper. Use an absolute tolerance of 1e-6.
add_cli_test(e2e_simple 0 FILE "${CMAKE_CURRENT_SOURCE_DIR}/examples/simple.opt"
    STATUS OPTIMAL TEXT "Method: MILP"
    VALUES "x,-0.000001,0.000001" "y,99.999999,100.000001"
           "profit,499.999999,500.000001")
add_cli_test(e2e_continuous 0 FILE "${CMAKE_CURRENT_SOURCE_DIR}/examples/continuous.opt"
    STATUS OPTIMAL TEXT "Method: LP"
    VALUES "x,-2.500001,-2.499999" "cost,-1.500001,-1.499999" OMIT y)

add_cli_test(e2e_variables_only 0 SOURCE [=[var x: real;]=]
    TEXT "Variables: 1" "Constraints: 0" "Objective: None" "Solve requested: No")
add_cli_test(e2e_objective_only 0 SOURCE [=[
var x: real;
maximize score: x;
]=] TEXT "Constraints: 0" "Objective: Declared" "Solve requested: No")
add_cli_test(e2e_constraints_only 0 SOURCE [=[
var x: real;
subject to:
    x >= 1;
]=] TEXT "Constraints: 1" "Objective: None" "Solve requested: No")

add_cli_test(e2e_constant_objective_zero_rows 0 SOURCE [=[
var x: real;
maximize score: 3 + x - x;
solve;
]=] STATUS OPTIMAL TEXT "Method: LP" "Constraints: 0"
    VALUES "score,2.999999,3.000001")
add_cli_test(e2e_fractional_integer_bounds 0 SOURCE [=[
var n: int;
maximize score: n;
subject to:
    n >= -1.5;
    n <= 2.5;
solve;
]=] STATUS OPTIMAL TEXT "Method: MILP"
    VALUES "n,1.999999,2.000001" "score,1.999999,2.000001")
add_cli_test(e2e_normalized_inequality 0 SOURCE [=[
var x: real;
var y: real;
maximize score: x;
subject to:
    2*x + 3 <= y + 8;
    y = 1;
solve;
]=] STATUS OPTIMAL VALUES "x,2.999999,3.000001" "y,0.999999,1.000001"
    "score,2.999999,3.000001")
add_cli_test(e2e_infeasible_lp 2 SOURCE [=[
var x: real;
minimize score: x;
subject to:
    x <= 0;
    x >= 1;
solve;
]=] STATUS INFEASIBLE TEXT "Method: LP")
add_cli_test(e2e_unbounded_lp 2 SOURCE [=[
var x: real;
maximize score: x;
solve;
]=] STATUS UNBOUNDED TEXT "Method: LP")
add_cli_test(e2e_uncertified_milp 2 SOURCE [=[
var x: int;
maximize score: x;
solve;
]=] STATUS UNKNOWN TEXT "Method: MILP")

add_cli_test(e2e_undeclared 1 SOURCE [=[
var x: real;
maximize score: x + missing;
]=] TEXT "at 2:21:" "undeclared identifier 'missing'")
add_cli_test(e2e_duplicate 1 SOURCE [=[
var x: real;
var x: int;
]=] TEXT "at 2:1:" "duplicate declaration of 'x'")
add_cli_test(e2e_parameter_cycle 1 SOURCE [=[
param a = b;
param b = a;
var x: real;
]=] TEXT "cyclic definition of parameter")
add_cli_test(e2e_parameter_variable 1 SOURCE [=[
param a = x;
var x: real;
]=] TEXT "cannot depend on non-parameter 'x'")
add_cli_test(e2e_nonlinear 1 SOURCE [=[
var x: real;
maximize score: (x + 1)*x;
]=] TEXT "nonlinear multiplication")
add_cli_test(e2e_strict_less 1 SOURCE [=[
var x: real;
subject to:
    lower: x < 1;
]=] TEXT "uses < or >")
add_cli_test(e2e_strict_greater 1 SOURCE [=[
var x: real;
subject to:
    upper: x > 1;
]=] TEXT "uses < or >")
add_cli_test(e2e_solve_no_objective 1 SOURCE [=[
var x: real;
solve;
]=] TEXT "at 2:1:" "no objective function declared")
add_cli_test(e2e_print_no_solve 1 SOURCE [=[
var x: real;
print x;
]=] TEXT "print requires a preceding solve")
add_cli_test(e2e_print_undeclared 1 SOURCE [=[
var x: real;
maximize score: 0;
solve;
print x, missing;
]=] TEXT "at 4:1:" "cannot print undeclared identifier 'missing'")
add_cli_test(e2e_print_constraint 1 SOURCE [=[
var x: real;
maximize score: x;
subject to:
    limit: x <= 1;
solve;
print limit;
]=] TEXT "'limit' is not a printable solution value")
add_cli_test(e2e_print_parameter 1 SOURCE [=[
param a = 1;
var x: real;
maximize score: a;
solve;
print a;
]=] TEXT "'a' is not a printable solution value")
add_cli_test(e2e_missing_semicolon 1 SOURCE [=[
var x: real
solve;
]=] TEXT "Syntax error at 2:1:")
add_cli_test(e2e_invalid_character 1 SOURCE [=[
# Leading comment and whitespace exercise scanner locations.
var x: real;
maximize score: x @ 1;
]=] TEXT "Lexical error at 3:19:" "'@'")
add_cli_test(e2e_statement_order 1 SOURCE [=[
var x: real;
param a = 1;
]=] TEXT "Syntax error at 2:1:")
add_cli_test(e2e_empty_variables 1 SOURCE [=[
param a = 1;
]=] TEXT "Syntax error at 2:1:")
add_cli_test(e2e_empty_constraints 1 SOURCE [=[
var x: real;
subject to:
]=] TEXT "Syntax error at 3:1:")
