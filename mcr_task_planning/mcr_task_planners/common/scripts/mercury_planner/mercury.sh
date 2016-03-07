#! /bin/bash

# --------------CHECK ARGS--------

# check if number of received arguments is ok, if not then prints USAGE instructions
if [[ $# != 3 ]]; then
    printf 'USAGE : domain_path problem_number mercury_path\n\n'
    echo '1 argument = path to domain                      (folder which contains problem.pddl file)'
    echo '2 argument = problem number #                    (asumes you have a folder which contains problems/p#.pddl)'
    echo '3 argument = path to mercury_planner binaries    (path to seq-sat-mercury code)'
    printf '\nEXAMPLE :\n'
    echo './mercury'
    echo '/home/user/indigo/src/mas_industrial_robotics/mir_task_planning/mir_knowledge/common/pddl/general_domain'
    echo '1'
    echo '/home/user/indigo/src/mas_third_party_software/mercury_planner/build/Mercury-fixed/seq-sat-mercury'
    printf '\nWARNING : Script execution will be aborted\n'
    exit 1
fi


#--------------SETUP--------------

# get the current directory
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Paths to planner components

# PDDL_DOMAIN="$BASEDIR/domain.pddl"
PDDL_DOMAIN_PATH="${1}/domain.pddl"

# PDDL_PROBLEM="$BASEDIR/p01.pddl"
PDDL_PROBLEM_PATH="${1}/problems/p${2}.pddl"

# the directoy which holds mercury_planner code
BASEDIR=${3}

# full path to translate.py file
TRANSLATE_PATH="$BASEDIR/src/translate/translate.py"

# full path to preprocess binary
PREPROCESS_PATH="$BASEDIR/src/preprocess/preprocess"

# full path to search binary
SEARCH_PATH="$BASEDIR/src/search/downward-1"

# command used to determine how much time took to plan
TIME="command time --output=elapsed.time --format=%S\n%U\n"

# command to run the python file
PYTHON_COMMAND="python"


#--------------RUN----------------
echo "1. Running translator"
$TIME $PYTHON_COMMAND $TRANSLATE_PATH $PDDL_DOMAIN_PATH $PDDL_PROBLEM_PATH

echo "2. Running preprocessor"
$TIME --append "$PREPROCESS_PATH" < output.sas

echo "3. Running search"
$TIME --append "$SEARCH_PATH" \
--heuristic "hrb=RB(cost_type=1, extract_plan=true, next_red_action_test=true, applicable_paths_first=true, use_connected=true)" \
--heuristic "hlm2=lmcount(lm_rhw(reasonable_orders=true,lm_cost_type=2,cost_type=2))" \
--search "lazy_wastar([hrb,hlm2],preferred=[hrb,hlm2],w=3)" \
--plan-file mercury.plan < output

#--------------CLEAN--------------

rm output
rm output.sas
rm plan_numbers_and_cost
