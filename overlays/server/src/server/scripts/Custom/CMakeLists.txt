# Custom/CMakeLists.txt

# Récupère automatiquement tous les .cpp du dossier Custom
file(GLOB_RECURSE CUSTOM_SCRIPTS
  "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)

# Les ajoute à la lib "scripts"
set(scripts_STAT_SRCS
  ${scripts_STAT_SRCS}
  ${CUSTOM_SCRIPTS}
)
