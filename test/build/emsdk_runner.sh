#!/bin/bash

# generated logs message
print_log() {
  msg="$1"
  echo -e "\n------- ${msg} -------\n"
}

# donwloads emscripten
download_and_install_emsdk() {
  path="$1"
  if [ -d ${path} ]; then
    cd ${path}

    if [ -d "emsdk" ]; then
      print_log "emsdk is already installed in the current directory"
      exit 0
    fi
    print_log "Installing emsdk at path: $(pwd)"

    git clone https://github.com/emscripten-core/emsdk.git
    cd emsdk

    git pull

    # Download and install the latest SDK tools.
    ./emsdk install latest

    # Make the "latest" SDK "active" for the current user. (writes .emscripten file)
    ./emsdk activate latest

    # Activate PATH and other environment variables in the current terminal
    source ./emsdk_env.sh
  else
    print_log "${path} not found"
  fi
}

target_file=""    # file in which the assembly target code is generated like javascript file
header_file=""    # header files to be included
main_file=""      # main_file for which the script has to be run
root_dir=""       # root directory where the main_file is present and the code will be generated here itself
emsdk_dir=""      # main directory in which the emscripten sources are installed

env_file="emsdk_runner_env.txt"       # file where the variable for default root_dir and others are stored
env_vars=()

check_flag=false  # just a flag used by functions

# sets the path where the emsdk directory is installed
set_emsdk_dir() {
  temp_emsdk_dir=$(cd ${root_dir} && cd .. && cd build && pwd)
  temp_emsdk_dir+="/emsdk/upstream/emscripten"
  if [ -d $emsdk_dir ]; then
    emsdk_dir=${temp_emsdk_dir}
  else
    print_log "${temp_emsdk_dir} directory was not found\nRun emsdk_runner.sh -i (for installing)"
    exit 1
  fi
}

# checks whether the main_file actually exists in the root_dir
check_for_correct_binaries() {
  for i in $(ls ${root_dir}); do
    if [ $i = ${main_file} ]; then
      check_flag=true
      break
    fi
  done

  if [ $check_flag = true ]; then
    print_log "Filename found"
    print_log "Executing Emscripten"
  else
    print_log "${main_file} was not found in the directory ${root_dir}"
  fi
}

# executed the empscripten script after all the checks
generate_using_emsdk() {
  emsdk_exec="${emsdk_dir}/emcc"
  cd .. && cd ${root_dir}
  
  print_log "Genereating emscripten files in $(pwd)"
  echo "Command: ${emsdk_exec} ${header_file} ${main_file} -o ${target_file}"

  $(${emsdk_exec} ${header_file} "${root_dir}/${main_file}" -o ${target_file})
}

# main function
run_emsdk() {
  main_file="$1"

  read -p "Enter the name of the directory (where emscripten file is to be executed): " root_dir
  read -p "Enter the target file name (app.js for example): " target_file
  read -p "Enter the header_file to be entered (if needed or else leave blank): " header_file

  # -z header_file returns true if the string is empty
  if [ ! -z $header_file ]; then
    header_file="--${header_file}"
  fi

  # -n header_file returns false if string is not empty
  if [ -n $root_dir ]; then
    dir_to_exec=$(cd .. && pwd)
    dir_to_exec+="/${root_dir}"
    root_dir=${dir_to_exec}

    # -d checks if the root_dir specified by the user, actually exists or not
    # returns true if it does and false otherwise
    if [ -d $root_dir ]; then
      print_log "Directory found"
      check_for_correct_binaries
      if [ $check_flag = true ]; then
        set_emsdk_dir
        generate_using_emsdk
        print_log "Emscripten executed successfully"
      fi
    else
      print_log "${root_dir} is not a directory"
    fi
  fi
}

create_env_variables() {
  echo -e "ROOT_DIR=${root_dir}\nHEADER_FILE=${header_file}\nTARGET_FILE=${target_file}" > ${env_file}
}

set_local_variables_using_env() {
  # IFS='='
  if [ ! -f ${env_file} ]; then
    run_emsdk "$1"
    create_env_variables
  fi

  # for line in $(cat ${env_file})
  # do
  #   echo -e "${line}\n"
  # done
}

while getopts "i:r:" execute; do
  main_file=$(echo $OPTARG)

  case "$execute" in
    i)
      download_and_install_emsdk "$main_file"
      ;;
    r)
      run_emsdk "$main_file"
      # set_local_variables_using_env "$main_file"
      ;;
    \?)
      echo "Invalid Input"
      print_log "Allowed input flags"
      echo "-i {relative path} : for installed in the emscripten packages"
      echo "-r {filename} : for running the script for the filename specified (for current directory enter '.')"
      ;;
  esac
done