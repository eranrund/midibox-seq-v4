# $Id: release.sh 1986 2014-05-03 21:30:22Z tk $

if [[ "$1" == "" ]]; then
  echo "SYNTAX: release.sh <release-directory>"
  exit 1
fi

RELEASE_DIR=$1

if [[ -e $RELEASE_DIR ]]; then
  echo "ERROR: the release directory '$RELEASE_DIR' already exists!"
  exit 1
fi

###############################################################################
echo "Creating $RELEASE_DIR"

mkdir $RELEASE_DIR
cp README.txt $RELEASE_DIR
cp CHANGELOG.txt $RELEASE_DIR

mkdir -p $RELEASE_DIR/hwcfg/standard_v4
cp hwcfg/standard_v4/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/standard_v4
mkdir -p $RELEASE_DIR/hwcfg/tk
cp hwcfg/tk/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/tk
mkdir -p $RELEASE_DIR/hwcfg/wilba
cp hwcfg/wilba/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/wilba
cp hwcfg/wilba_tpd/MBSEQ_HW.V4 $RELEASE_DIR/hwcfg/wilba_tpd
cp hwcfg/README.txt $RELEASE_DIR/hwcfg

###############################################################################
echo "Building for MBHP_CORE_STM32"

make cleanall
export MIOS32_FAMILY=STM32F10x
export MIOS32_PROCESSOR=STM32F103RE
export MIOS32_BOARD=MBHP_CORE_STM32
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/MBHP_CORE_STM32
make > $RELEASE_DIR/MBHP_CORE_STM32/log.txt
cp project.hex $RELEASE_DIR/MBHP_CORE_STM32

###############################################################################
echo "Building for MBHP_CORE_LPC17"

make cleanall
export MIOS32_FAMILY=LPC17xx
export MIOS32_PROCESSOR=LPC1769
export MIOS32_BOARD=MBHP_CORE_LPC17
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/MBHP_CORE_LPC17
make > $RELEASE_DIR/MBHP_CORE_LPC17/log.txt
cp project.hex $RELEASE_DIR/MBHP_CORE_LPC17

###############################################################################
echo "Building for MBHP_CORE_STM32F4"

make cleanall
export MIOS32_FAMILY=STM32F4xx
export MIOS32_PROCESSOR=STM32F407VG
export MIOS32_BOARD=MBHP_CORE_STM32F4
export MIOS32_LCD=universal
mkdir -p $RELEASE_DIR/$MIOS32_BOARD
make > $RELEASE_DIR/$MIOS32_BOARD/log.txt
cp project.hex $RELEASE_DIR/$MIOS32_BOARD

###############################################################################
make cleanall
echo "Done!"

