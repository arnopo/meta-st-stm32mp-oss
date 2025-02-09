SUMMARY = "OPTEE TA development kit for stm32mp"
LICENSE = "BSD-2-Clause & BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=c1f21c4f72f372ef38a5a4aee55ec173"

SRC_URI = "git://github.com/OP-TEE/optee_os.git;protocol=https;name=os"

PV = "${OPTEE_VERSION}-git${SRCPV}"

S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "(stm32mpcommon)"

OPTEEMACHINE ?= "stm32mp1"
OPTEEMACHINE_stm32mp1common = "stm32mp1"

OPTEEOUTPUTMACHINE ?= "stm32mp1"
OPTEEOUTPUTMACHINE_stm32mp1common = "stm32mp1"

PACKAGE_ARCH = "${MACHINE_ARCH}"
PROVIDES += "virtual/optee-os"
RPROVIDES_${PN} += "virtual/optee-os virtual/systemd-bootconf"

B = "${WORKDIR}/build"
# Configure build dir for externalsrc class usage through devtool
EXTERNALSRC_BUILD_pn-${PN} = "${WORKDIR}/build"

DEPENDS += "dtc-native python3-pycryptodomex-native python3-pycrypto-native python3-pyelftools-native libgcc"
DEPENDS += "python3-cryptography-native"

inherit deploy python3native

OPTEEMACHINE ?= "${MACHINE}"
OPTEEOUTPUTMACHINE ?= "${MACHINE}"

EXTRA_OEMAKE = "CROSS_COMPILE_core=${HOST_PREFIX} \
                CROSS_COMPILE_ta_arm64=${HOST_PREFIX} \
                ${@bb.utils.contains('TUNE_FEATURES', 'aarch64', 'CFG_ARM64_core=y ta-targets=ta_arm64', 'CFG_ARM32_core=y CROSS_COMPILE_ta_arm32=${HOST_PREFIX}', d)} \
                NOWERROR=1 \
                LDFLAGS= \
        "

EXTRA_OEMAKE += "CFG_TEE_CORE_LOG_LEVEL=2"
EXTRA_OEMAKE += "CFG_TEE_CORE_DEBUG=n"

OPTEE_ARCH_armv7a = "arm32"
OPTEE_ARCH_armv7ve = "arm32"
OPTEE_ARCH_aarch64 = "arm64"

do_configure_prepend(){
    chmod 755 ${S}/scripts/bin_to_c.py
}

do_compile() {
    export CFLAGS="${CFLAGS} --sysroot=${STAGING_DIR_HOST}"

    unset -v CPPFLAGS LDFLAGS LDADD
    if [ -n "${OPTEE_CONF_EXTENDED}" ]; then
        for conf in ${OPTEE_CONF_EXTENDED}; do
            predtb=$(echo ${conf} | cut -d':' -f1)
            param=$(echo ${conf} | cut -d':' -f2)
            oe_runmake -C ${S} O=${B}/$predtb $param CFG_EMBED_DTB_SOURCE_FILE=${predtb}.dts
        done
    else
        if [ -n "${OPTEE_CONF}" ]; then
            for conf in ${OPTEE_CONF}; do
                oe_runmake -C ${S} O=${B}/${conf} PLATFORM=${OPTEEMACHINE} CFG_EMBED_DTB_SOURCE_FILE=${conf}.dts
            done
        else
            oe_runmake -C ${S} O=${B}/out
        fi
    fi
}

do_install() {
    #install TA devkit
    install -d ${D}${includedir}/optee/export-user_ta/

    if [ -n "${OPTEE_CONF_EXTENDED}" ]; then
        for conf in ${OPTEE_CONF_EXTENDED}; do
            predtb=$(echo ${conf} | cut -d':' -f1)
            for f in  ${B}/$predtb//export-ta_${OPTEE_ARCH}/* ; do
                cp -aRf  $f ${D}${includedir}/optee/export-user_ta/
            done
        done
    else
        if [ -n "${OPTEE_CONF}" ]; then
            for conf in ${OPTEE_CONF}; do
                for f in  ${B}/${conf}/export-ta_${OPTEE_ARCH}/* ; do
                    cp -aRf  $f ${D}${includedir}/optee/export-user_ta/
                done
            done
        fi
    fi
}

# Configure optee binaries
OPTEE_BOOTCHAIN = "optee"
OPTEE_HEADER    = "tee-header_v2"
OPTEE_PAGEABLE  = "tee-pageable_v2"
OPTEE_PAGER     = "tee-pager_v2"
OPTEE_SUFFIX    = "bin"
# Output the ELF generated
ELF_DEBUG_ENABLE ?= ""
OPTEE_ELF = "tee"
OPTEE_ELF_SUFFIX = "elf"

do_deploy[sstate-outputdirs] = "${DEPLOY_DIR_IMAGE}/optee"
do_deploy() {
    install -d ${DEPLOYDIR}
    if [ -n "${OPTEE_CONF_EXTENDED}" ]; then
        for conf in ${OPTEE_CONF_EXTENDED}; do
            predtb=$(echo ${conf} | cut -d':' -f1)
            install -m 644 ${B}/$predtb/core/${OPTEE_HEADER}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_HEADER}-$predtb-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
            install -m 644 ${B}/$predtb/core/${OPTEE_PAGER}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_PAGER}-$predtb-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
            install -m 644 ${B}/$predtb/core/${OPTEE_PAGEABLE}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_PAGEABLE}-$predtb-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
            if [ -n "${ELF_DEBUG_ENABLE}" ]; then
                install -m 644 ${B}/$predtb/core/${OPTEE_ELF}.${OPTEE_ELF_SUFFIX} ${DEPLOYDIR}/${OPTEE_ELF}-$predtb-${OPTEE_BOOTCHAIN}.${OPTEE_ELF_SUFFIX}
            fi
        done
    else
        if [ -n "${OPTEE_CONF}" ]; then
            for conf in ${OPTEE_CONF}; do
                install -m 644 ${B}/${conf}/core/${OPTEE_HEADER}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_HEADER}-${conf}-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
                install -m 644 ${B}/${conf}/core/${OPTEE_PAGER}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_PAGER}-${conf}-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
                install -m 644 ${B}/${conf}/core/${OPTEE_PAGEABLE}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_PAGEABLE}-${conf}-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
                if [ -n "${ELF_DEBUG_ENABLE}" ]; then
                    install -m 644 ${B}/${conf}/core/${OPTEE_ELF}.${OPTEE_ELF_SUFFIX} ${DEPLOYDIR}/${OPTEE_ELF}-${conf}-${OPTEE_BOOTCHAIN}.${OPTEE_ELF_SUFFIX}
                fi
            done
        else
            install -m 644 ${B}/core/${OPTEE_HEADER}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_HEADER}-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
            install -m 644 ${B}/core/${OPTEE_PAGER}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_PAGER}-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
            install -m 644 ${B}/core/${OPTEE_PAGEABLE}.${OPTEE_SUFFIX} ${DEPLOYDIR}/${OPTEE_PAGEABLE}-${OPTEE_BOOTCHAIN}.${OPTEE_SUFFIX}
            if [ -n "${ELF_DEBUG_ENABLE}" ]; then
                install -m 644 ${B}/core/${OPTEE_ELF}.${OPTEE_ELF_SUFFIX} ${DEPLOYDIR}/${OPTEE_ELF}-${OPTEE_BOOTCHAIN}.${OPTEE_ELF_SUFFIX}
            fi
        fi
    fi
}
addtask deploy before do_build after do_compile

FILES_${PN} = "${nonarch_base_libdir}/firmware/"
FILES_${PN}-dev = "/usr/include/optee"

INSANE_SKIP_${PN}-dev = "staticdev"

INHIBIT_PACKAGE_STRIP = "1"

# The package is empty but must be generated to avoid apt-get installation issue
ALLOW_EMPTY_${PN} = "1"

