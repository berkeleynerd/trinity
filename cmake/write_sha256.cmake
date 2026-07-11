if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "write_sha256.cmake requires INPUT and OUTPUT")
endif()

file(SHA256 "${INPUT}" INPUT_SHA256)
file(WRITE "${OUTPUT}" "${INPUT_SHA256}  ${INPUT}\n")
