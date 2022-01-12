project('hex', 'c', license : 'BSD-3-Clause')

##########################
# Dependencies and tools #
##########################

pkgconfig = import('pkgconfig')

lua = dependency('lua', version : '>=5.4')

subdir('tools/bin2src')

###########
# Targets #
###########

subdir('include')
subdir('src/libhex')
subdir('src/hex')

##################
# Documentations #
##################

subdir('docs')
