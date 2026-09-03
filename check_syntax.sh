#!/bin/bash

# check_syntax.sh
# Performs a syntax and semantic check on a C source file using clang -fsyntax-only.
# Arguments:
#   $1: Path to the C source file (typically the preprocessed .i file from cetus_output)
#
# Returns:
#   0 on success (syntax/semantics are valid)
#   Non-zero on failure (syntax/semantics are invalid, clang will print errors to stderr)

INPUT_FILE="$1"

# Basic argument validation
if [ -z "$INPUT_FILE" ]; then
    echo "Usage: $0 <path_to_c_file>" >&2
    exit 1
fi

# Check if the input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found." >&2
    exit 1
fi

# Perform the syntax and semantic check using clang -fsyntax-only.
# -std=c11 ensures C11 compliance.
# -Werror treats all warnings as errors, making the check stricter.
clang -fsyntax-only -std=c11 -Werror "$INPUT_FILE"

# Check the exit status of the clang command.
# $? holds the exit status of the last executed command.
if [ $? -eq 0 ]; then
    # echo "Syntax and semantic check passed for '$INPUT_FILE'." # Optional success message
    exit 0 # Success
else
    # clang already prints detailed error messages to stderr, so no need for custom error.
    exit 1 # Failure
fi