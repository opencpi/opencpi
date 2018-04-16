#!/bin/bash
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

# Generate PDFs from all tex, odt files
# When run by jenkins this script will generate all PDFs using
# makepdf.sh and then makepdf.sh will call optpdf.sh to shrink the size
# of the files.  All three of these .sh files should be located in
# the same directory. Finally after the PDFs are generated and moved
# to a specfic output location an index.html will be generated listing
# all of the PDFs in the same output location.

## Enables color variables BOLD, RED, and RESET
enable_color() {
  if [ -n "$(command -v tput)" ]; then
    export BOLD=$(tput bold)
    export RED=$(tput setaf 1)
    export RESET=$(tput sgr0)
  fi
}

###
# Add a new section to the table in index.html
# Globals:
#   None
# Arguments:
#   $1: NAME
#       Name of the documentation
#   $2: LOCATION
#       The locations of the PDFs to be added
#       to the table
# Returns:
#   An html table of documents to standard output
###
add_new_table_section() {
    # If directory is empty do not create an entry for it
    if compgen -o filenames -G $2/'*.pdf' > /dev/null; then
        count=0
        tableCols=3
        echo "<tr><th colspan=\"${tableCols}\">$1</th></tr>"
        for pdf in $2/*.pdf; do
            file_name=$(basename -s .pdf $pdf)
            if [ $count -eq 0 ]; then
                echo "<tr>"
            fi
            echo '<td><a href="'${pdf}'">'$(echo ${file_name} | tr '_' ' ')'</a></td>'
            count=$(((${count} + 1) % ${tableCols}))
            if [ $count -eq 0 ]; then
                echo "</tr>"
            fi
        done
        if [ $count -ne 0 ]; then
            echo "</tr>"
        fi
    fi
}

###
# Created index.html with links to documents
# Globals:
#   OUTPUT_PATH
#       The path where the PDFs are to be written
#   BSPS
#        Array of the avaliable bsps
#   MYLOC
#       The path of the directory that contains this script
# Arguments:
#   None
# Returns:
#   All of the documents in an html file to standard out
###
create_index() {
    cd ${OUTPUT_PATH}
    av_pdf_loc="."
    asset_pdf_loc="./assets"
    core_pdf_loc="./core"

    # Bring in start of index.html
    cat ${MYLOC}/listing_header.html

    add_new_table_section "AV Team Main Documentation" "${av_pdf_loc}"
    add_new_table_section "Assets Project Documentation" "${asset_pdf_loc}"
    add_new_table_section "Core Project Documentation" "${core_pdf_loc}"

    for d in ${BSPS[*]}; do
        add_new_table_section "${d^} BSP Documentation" "./bsp_$d"
    done

    # Bring in ending of index.html
    cat ${MYLOC}/listing_footer.html
}

###
# Convert files to PDFs and if it is converting
# tex docs make sure to shrink in size
# Globals:
#   MYLOC
#       The path of the directory that contains this script
#   OUTPUT_PATH
#       The path where the PDFs are to be written
#   REPO_PATH
#       The path where the opencpi repo is located
#   BSPS
#       Array of the avaliable bsps
# Arguments:
#   None
# Returns:
#   None
#       Some stuff will be returned but it is all garbage and
#       should not be used.
###
generate_pdfs() {
    shopt -s nullglob
    enable_color

    echo "${BOLD}Building PDFs from '${REPO_PATH}' with results in '${OUTPUT_PATH}'${RESET}"

    # Set up output area
    mkdir -p ${OUTPUT_PATH}/{,assets,core}/logs
    for d in ${BSPS[*]}; do
        mkdir -p ${OUTPUT_PATH}/bsp_$d/logs
    done

    # Get around unoconv bug ( https://github.com/dagwieers/unoconv/issues/241 )
    cd /tmp; unoconv -vvv -f pdf /dev/null >/dev/null 2>&1 || :

    dirs_to_search=()
    dirs_to_search+=("${REPO_PATH}/doc/av/tex")
    dirs_to_search+=("${REPO_PATH}/doc/odt")
    dirs_to_search+=($(find ${REPO_PATH}/projects/assets -type d -name doc))
    dirs_to_search+=($(find ${REPO_PATH}/projects/bsps -type d -name doc))
    dirs_to_search+=($(find ${REPO_PATH}/projects/core -type d -name doc))
    # The benchmarking files are not in a doc dir but are in a docs dir
    dirs_to_search+=($(find ${REPO_PATH}/projects/assets -type d -name docs))

    for d in ${dirs_to_search[*]}; do
        echo "${BOLD}Directory: $d${RESET}"
        cd $d
        if expr match $d '.*assets' > /dev/null; then
            prefix=assets
        elif expr match $d '.*core' > /dev/null; then
            prefix=core
        else
            # If it is a BSP, set prefix to bsp_<bspsname> else set prefix to ""
            prefix=$(echo $d | perl -ne 'print "bsp_$1\n" if m|/bsps/(.+?)/|')
        fi

        log_dir=${OUTPUT_PATH}/${prefix}/logs

        for ext in *docx *pptx *odt *fodt *odp *fodp; do
            # Get the name of the file without the file extension
            ofile=${ext%.*}
            echo "${BOLD}office: $d/$ext${RESET}"
            unoconv -vvv -f pdf $ext >> ${log_dir}/${ofile}.log 2>&1
            # If the pdf was created then try to shrink it
            if [ -f $ofile.pdf ]; then
                echo "Original $(stat -c "%s" ${ofile}.pdf) bytes"
                while ${MYLOC}/optpdf.sh ${ofile}.pdf;do :;done
                mv ${ofile}.pdf ${OUTPUT_PATH}/${prefix}/
            else
              echo "${BOLD}${RED}Error creating $ofile.pdf${RESET}"
              echo "Error creating $ofile.pdf ($d)" >> ${OUTPUT_PATH}/errors.log
            fi
        done
        for tex in *tex; do
            ofile=$(basename -s .tex $tex)
            echo "${BOLD}LaTeX: $d/$tex${RESET}"
            rubber -d $tex
            # If the pdf was created then try to shrink it
            if [ -f $ofile.pdf ]; then
                echo "Original $(stat -c "%s" ${ofile}.pdf) bytes"
                while ${MYLOC}/optpdf.sh ${ofile}.pdf;do :;done
            else
              echo "${BOLD}${RED}Error creating $ofile.pdf${RESET}"
              echo "Error creating $ofile.pdf ($d)" >> ${OUTPUT_PATH}/errors.log
            fi

            mv ${ofile}.log ${log_dir}/${ofile}.log 2>&1

            rubber-info --boxes $tex >> ${log_dir}/${ofile}_boxes.log 2>&1
            rubber-info --check $tex >> ${log_dir}/${ofile}_warnings.log 2>&1
            rubber-info --warnings $tex >> ${log_dir}/${ofile}_warnings.log 2>&1

            rm -f ${ofile}.{aux,out,log,lof,lot,toc,dvi,synctex.gz}

            mv ${ofile}.pdf ${OUTPUT_PATH}/${prefix}/
        done
    done
}

###
# Sends help message to STDERR
# Globals:
#   None
# Arguments:
#   $1: Error string to print before help (optional)
# Returns:
#   Ends script with success or failure depending on $1 presence
###
show_help() {
    enable_color
    [ -n "$1" ]  && printf "\n${RED}ERROR: %s\n\n${RESET}" "$1"
    cat <<EOF >&2
${BOLD}This genDocumentation script creates PDFs for OpenCPI.
Usage is: $0 [-h] [-r] [-o]

  -r / --repopath          Use the repo location provided
  -o / --outputpath        Use the provided location for output
  -h / --help              Display the help screen${RESET}
EOF
    [ -n "$1" ] && exit 99
    exit 0
}

# Figure out where we were called from in relation to caller
# Exporting MYLOC so when we call otpdf.sh it can find pdfmarks
export MYLOC=$(readlink -e "$(dirname $0)")

# Defaults:
REPO_PATH="$(readlink -e .)"
OUTPUT_PATH="${REPO_PATH}/pdf/"
index_file="index.html"

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -h|--help)
            show_help
            ;;
        -r|--repopath)
            REPO_PATH=$(readlink -e "$2")
            shift # past argument
            shift # past value
            ;;
        -o|--outputpath)
            OUTPUT_PATH=$(readlink -f "$2")
            shift # past argument
            shift # past value
            ;;
        *)  # unknown option
            show_help "Unknown argument $key"
            ;;
    esac
done
# Do not try to access parameters to the script past this

[ ! -r ${REPO_PATH} ] && show_help "\"${REPO_PATH}\" not readable by $USER. Consider using a different repo path."

[ ! -d "${REPO_PATH}/doc/av" ] && show_help "\"${REPO_PATH}\" doesn't seem to be correct. Could not find doc/av/."

[ -d "${OUTPUT_PATH}" ] && show_help "\"${OUTPUT_PATH}\" already exists"

mkdir -p ${OUTPUT_PATH} > /dev/null 2>&1
touch ${OUTPUT_PATH}/${index_file} > /dev/null 2>&1
[ ! -w ${OUTPUT_PATH}/${index_file} ] && show_help "\"${OUTPUT_PATH}\" not writable by $USER. Consider using a different output path."

BSPS=()
BSPS+=($(find ${REPO_PATH}/projects/bsps/ -mindepth 1 -maxdepth 1 \
            -type d -printf '%f\n'))

generate_pdfs
create_index > ${OUTPUT_PATH}/${index_file}

# If errors...
if [ -f ${OUTPUT_PATH}/errors.log ]; then
  # Fail if it's not a "special" file
  if [ -n "$(grep -v _header ${OUTPUT_PATH}/errors.log | grep -v _footer | grep -v _template)" ]; then
    echo "Errors were detected!"
    cat ${OUTPUT_PATH}/errors.log
    exit 2
  fi
  # Must be only special files
  rm -f ${OUTPUT_PATH}/errors.log
fi
