astyle --style=allman --indent=spaces -R "${PWD}/*.h" "${PWD}/*.cpp" "${PWD}/*.ino" --unpad-paren / -U --indent-switches / -S
astyle --style=allman --indent=spaces -R "${PWD}/*.h" "${PWD}/*.cpp" "${PWD}/*.ino" --pad-header / -H --indent-switches / -X

