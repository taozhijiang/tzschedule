aux_source_directory(./ DIR_LIB_SRCS)
add_library ( jobdefer SHARED ${DIR_LIB_SRCS})

include_directories(
           ../xtra_rhelz.x/include )
