 ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk 


# binary name
APP = main

# all source are stored in SRCS-y
SRCS-y :=main.c  


CFLAGS += -O0 -g  -I$(SRCDIR) -I$(Mysql)

CFLAGS += $(WERROR_FLAGS)

#LDLIBS=/usr/lib64/mysql/libmysqlclient.so		
#LDLIBS += -L$(subst ethtool-app,lib,$(RTE_OUTPUT))/lib
#LDLIBS += -lrte_ethtool

LDLIBS += -lrte_pmd_e1000 -lrte_pmd_i40e -lrte_pmd_ixgbe
		
ifeq ($(CONFIG_RTE_BUILD_SHARED_LIB),y)
ifeq ($(CONFIG_RTE_LIBRTE_IXGBE_PMD),y)
LDLIBS += -lrte_pmd_ixgbe
endif
endif


#CFLAGS += -I$(SRCDIR)
#CFLAGS += -O3 -g $(USER_FLAGS)
#CFLAGS += $(WERROR_FLAGS)

include $(RTE_SDK)/mk/rte.extapp.mk
