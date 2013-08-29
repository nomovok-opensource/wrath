 #!/bin/bash

###########################################################
#
# This script will automatically generate the doxy
# pages related to the examples code. It will generate
# for each example in the examples folder:
#
#  + a main page with a description
#  + links to the source files and shaders
#  + a screenshot of the example
#
# You don't need to call this script manually, it is
# called automatically every time you "make docs".
# However in case you want to call it directly with
# custom parameters the usage is as follows:
#
# generate_example_docs arg1 arg2 arg3
#
# arg1 = PATH_TO_EXAMPLES_DIR  
# arg2 = PATH_TO_OUTPUT_DIR 
# arg3 = PATH_TO_SCREENSHOTS_DIR
#
###########################################################
write_header() {
   str0="/////////////"
   str1=$( echo $1 | tr -s '[:lower:]' '[:upper:]' )
   str2=" EXAMPLE"
   str="$str0$str1$str2$str0"
   echo $str>>$2
}

#main page for all examples
write_main() {
local __DIRS=$1
local __OUT=$2
 echo "/*!">>$__OUT
 echo "\page wrath_examples_main WRATH Examples">>$__OUT
 for DIR in $__DIRS
    do
    local __DIRNAME=`basename $DIR`
        if [ ! $__DIRNAME = "common" ] 
        then
            echo "- \subpage "$__DIRNAME"_example">>$__OUT
        fi
    done  
 echo "*/">>$__OUT
}

#main page for a given example
write_example_main() {
    local __DIR=$1
    local __DIRNAME=$2
    local __PIC_DIR=$3
    local __OUT=$4
    echo "/*!">>$__OUT
    echo "\page "$__DIRNAME"_example "$__DIRNAME" example">>$__OUT
    echo >>$__OUT
    echo "\b Files:">>$__OUT
    echo >>$__OUT
    write_subpages $__DIR $__DIRNAME $__OUT
    write_description $__DIR $__DIRNAME $__OUT
    if [ -e "$__PIC_DIR/"$__DIRNAME".png" ] 
    then
        echo "\image html "$__DIRNAME".png">>$__OUT
    else
        echo "\image html noimage.png">>$__OUT
    fi

    echo "*/">>$__OUT
}

write_subpages() {
    local __DIR=$1
    local __DIRNAME=$2
    local __OUT=$3

    local __SRCS=$(get_sources $__DIR)        
    for SRC in $__SRCS
        do
            local __SRCNAME=`basename $SRC`
            local __REP=${__SRCNAME//./_}
            echo "- \subpage "$__DIRNAME"_"$__REP>>$__OUT
        done
    
    local __SHDRS=$(get_shaders $__DIR"/shaders")
    for SHDR in $__SHDRS
        do
            local __SHDRNAME=`basename $SHDR`
            local __REP=${__SHDRNAME//./_}
            echo "- \subpage "$__DIRNAME"_"$__REP>>$__OUT
        done

    echo>>$__OUT

}

write_description() {
    local __DIR=$1
    local __DIRNAME=$2
    local __OUT=$3

    local __F=$__DIR"/"$DIRNAME".cpp"

    if [ -e $__F ] 
    then
        #this was extremely painful, I couldn't find a simpler way to do it.
        #tr removes temporarily the linebreaks so grep can find multiline patterns
        #the intermediate grep is to make sure that if the details section doesnt exist sed won't blow it by returning the whole input

        #extract the \details text from cpp file
        local __DESC=$( cat $__F | tr '\n' '\a' | grep -P -o -e '\\details.*?\*/' | sed -r 's|\\details(.*)\*/|\1|g' | tr '\a' '\n')
        echo "$__DESC">>$__OUT
    fi
}

write_file() {
    local __OUTFILE=$4
    local __NAME=$3
    local __DIR=$2
    local __SRC=$1
    #replace . with _ 
    local __REP=${__SRC//./_}
    echo "/*!">>$__OUT
    echo "\page "$__NAME"_"$__REP" "$__DIR"/"$__SRC>>$__OUT
    echo "\includelineno "$__DIR"/"$__SRC>>$__OUT
    echo "*/">>$__OUT
    echo >>$__OUT
}

write_sources() {
    local __DIR=$1
    local __DIRNAME=$2
    local __OUT=$3
    #list of hpp and cpps in DIR
    local __SRCS=$(get_sources $__DIR)        
    for SRC in $__SRCS
        do
            local __SRCNAME=`basename $SRC`
            write_file $__SRCNAME $__DIRNAME $__DIRNAME $__OUT
        done
}

write_shaders() {
    local __DIR=$1
    local __DIRNAME=$2
    local __OUT=$3
    #list of files in shaders folder
    local __SHDRS=$(get_shaders $__DIR"/shaders")
    for SHDR in $__SHDRS
        do
            local __SHDRNAME=`basename $SHDR`
            write_file $__SHDRNAME $__DIRNAME"/shaders" $__DIRNAME $__OUT
        done
}

get_subdirs() {
    if [ -d "$1" ]
    then
        find $1 -maxdepth 1 -mindepth 1 -type d | sort
    fi
}

get_sources() {
    if [ -d "$1" ] 
    then
        find  $1 -maxdepth 1 -mindepth 1 -name \*.[ch]pp | sort
    fi 
}

get_shaders() {
    if [ -d "$1" ] 
    then
        find $1 -maxdepth 1 -mindepth 1 -name \*.glsl | sort
    fi
}

#remove trailing slash from a string
no_slash() {
    echo "$1" | sed -r "s|\/$||g" 
}

print_help() {
    echo "usage: $0 PATH_TO_EXAMPLES_DIR  PATH_TO_OUTPUT_DIR PATH_TO_SCREENSHOTS_DIR"
}

#check cli arguments
check_args() {
    if [ -z "$1" ] || [ -z "$2" ] || [ -z "$3" ]
    then
        print_help
        exit 0
    elif [ $1 == "--help" ]
    then
        print_help
        exit 0
    fi
}

check_args $1 $2 $3

#main examples directory
if [ -n $1 ] && [ -d $1 ]
then
    #remove trailing slash if applicable
    EXAMPLES_DIR=$( no_slash "$1" )
else
    echo $0" error: Invalid examples directory."
    exit 1
fi

#second argument is path to output folder
if [ -n $2 ] && [ -d $2 ] 
then
    NO_SLASH=$( no_slash "$2" )
    FILE=$NO_SLASH"/all_examples.doxy"
    TEMP=$FILE".tmp"
    if [ -e $TEMP ]; then
        rm $TEMP
    fi
    touch $TEMP
else
    echo $0" error: invalid output directory"
    exit 1
fi

#third argument the path to the screenshots folder
if [ -n $3 ]  && [ -d $3 ] 
then
    PICDIR=$( no_slash "$3" )
else
    echo $0" error: invalid screenshots directory"
    exit 1
fi

NOW=$(date -d @`date +%s` "+%T on %m/%d/%Y")
echo "///////////////////////////////////////////////////////////////////////">>$TEMP
echo "//">>$TEMP
echo "// This file was auto generated @"$NOW" do not modify">>$TEMP
echo "//">>$TEMP
echo "///////////////////////////////////////////////////////////////////////">>$TEMP

#list of all example directories
DIRS=$(get_subdirs "$EXAMPLES_DIR")
write_main "$DIRS" "$TEMP"

for DIR in $DIRS 
    do
        DIRNAME=`basename $DIR`
        #skip common folder
        if [ ! $DIRNAME = "common" ] 
        then
            write_header $DIRNAME $TEMP
            write_example_main $DIR $DIRNAME $PICDIR $TEMP
            write_sources $DIR $DIRNAME $TEMP
            write_shaders $DIR $DIRNAME $TEMP
        fi
    done

mv $TEMP $FILE
