# Create an INTERFACE library for our CPP module.
add_library(usermod_wireguard INTERFACE)

# Add our source files to the library.
target_sources(usermod_wireguard INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/WireGuard/src/crypto/refc/x25519.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_wireguard INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(wireguard INTERFACE usermod_wireguard)
