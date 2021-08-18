def usage_msg():
    """
    brief - meant to be displayed to user the in order to get an understanding of how to
    operate the submitter application when executed with the '-h' or '--help' flag
    param(s) - N/A
    return - N/A
    """
    
    print("|+++++++++++++++++++++++++++++++HELP MANUAL+++++++++++++++++++++++++++++++|")
    print("|\n| !!! all arguments after -n flag must be wrapped in quotes")
    print("|\n| NOTE: if using multiple operands, they must be comma delimitted\n|")
    print('| usage: submitter.py -n "[operand(s)] [op chain] '\
                                '[iterations (always 1)]"')
    print("| for single operand usage:")
    print('|\texample: <submitter.py -n "7" "6-,|8,=>>4 1>"')
    print("| for number ranges:")
    print('|\texample: <submitter.py -n "1-4" "+3,-4,^6 1>"')
    print("| for multi number requests:")
    print('|\texample: <submitter.py -n "1,4,7" "+1,+2,-3,&37 1>"')
    print("|\n| NOTE: if using -s 'shutdown flag' nothing should follow\n|")
    print("|+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++|")