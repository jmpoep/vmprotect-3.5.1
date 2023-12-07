#!/bin/bash
set -e

osascript -e 'tell application "Finder" to eject (every disk whose ejectable is true)'
VERSION_NUMBER="1.0 beta"

VERSION="$0 version ${VERSION_NUMBER}
Original name: make_dmg.sh
Author: Valentine Silvansky <v.silvansky@gmail.com>"

LICENSE="This software is distibuted under GNU GPLv3 license.
Visit http://www.gnu.org/licenses/gpl-3.0.txt for more information."

HELP="Usage:
  $0 [options...] bundle_name
  $0 -v
  $0 -h
  $0 -l

Available options:
  -d <folder>         Specify folder with a bundle. Defaults to pwd.
  -i <file>           Specify icon file to set to resulting dmg, contains absolute or relative path.
  -b <file>           Specify background image file to set to resulting dmg, contains absolute or relative path.
  -c <coordinates>    Specify coordinates to set to Applications symlink and Bundle in dmg, should be in format \"ApplicationsX:ApplicationsY:BundleX:BundleY\". If not specified, icons are unarranged.
  -s <size>           Specify dmg window size in Finder, should be in format \"WindowWidth:WindowHeight\". If not specified, defaults to 640x480.
  -n <name>           Specify resulting dmg name. If not specified, defaults to BundleName.dmg.
  -N <name>           Specify resulting dmg's volume name. If not specified, defaults to \"BundleName\".
  -t <folder>         Specify temporary dir. If not specified, defaults to /tmp.
  -S <identity>       Codesign bundle with specified identity before making dmg. If this option is not used, no signing performed. Warning! This will replace existing code signature if any!
  -V                  Add bundle version to dmg name as a suffix. Version is read from bundle's Info.plist file.

  -v                  Print version acnd copyright and exit.
  -h                  Print help and exit.
  -l                  Print license info and exit."

ARG_DIR=`pwd`
ARG_ICON=
ARG_BACKGROUND=
ARG_COORDS=
ARG_SIZE="640:480"
ARG_DMG_NAME=
ARG_VOL_NAME=
ARG_TMP_DIR="/tmp/make_dmg"
ARG_ADD_VERSION=
ARG_CODESIGN_ID=

# reading options
while getopts ":d:i:b:c:s:N:n:t:Vvhl" opt; do
	case $opt in
	v)
		echo "${VERSION}"
		exit 0
		;;
	l)
		echo "${LICENSE}"
		exit 0
		;;
	h)
		echo "${HELP}"
		exit 0
		;;
	d)
		echo "Setting target dir to $OPTARG"
		ARG_DIR=$OPTARG
		;;
	i)
		echo "Setting icon to $OPTARG"
		ARG_ICON=$OPTARG
		;;
	b)
		echo "Setting background to $OPTARG"
		ARG_BACKGROUND=$OPTARG
		;;
	c)
		echo "Setting coordinates to $OPTARG"
		ARG_COORDS=$OPTARG
		;;
	s)
		echo "Setting window size to $OPTARG"
		ARG_SIZE=$OPTARG
		;;
	n)
		echo "Setting dmg name to $OPTARG"
		ARG_DMG_NAME=$OPTARG
		;;
	N)
		echo "Setting dmg volume name to $OPTARG"
		ARG_VOL_NAME=$OPTARG
		;;
	t)
		echo "Setting temporary dir to $OPTARG"
		ARG_TMP_DIR=$OPTARG
		;;
	S)
		echo "Setting codesign identity to $OPTARG"
		ARG_CODESIGN_ID=$OPTARG
		;;
	V)
		echo "Enabling version info in resulting dmg"
		ARG_ADD_VERSION=1
		;;
	\?)
		echo "Invalid option: -$OPTARG"
		exit 1
		;;
	:)
 		echo "Error! Option -$OPTARG requires an argument."
		exit 1
		;;
	esac
done

shift $(($OPTIND - 1))

ARGS=$*
set -- $ARGS

APP_BUNDLE_NAME="$@"

if [ "$ARGS" ]; then
	echo "Bundle name set to ${APP_BUNDLE_NAME}";
else
	echo "Error! Bundle name is not specified."
	exit 1;
fi

if [ ! -e "${APP_BUNDLE_NAME}" ]; then
	echo "Error! Bundle \"${APP_BUNDLE_NAME}\" does not exist!"
	exit 1
fi

TARGET_DIR=${ARG_DIR}

if [ ! "${TARGET_DIR}" ]; then
	TARGET_DIR=`pwd`
fi

if [ ! -e "${TARGET_DIR}" ]; then
	echo "Error! Directory \"${TARGET_DIR}\" does not exist."
	exit 1
fi

TMP_DIR=${ARG_TMP_DIR}

VOL_NAME=${ARG_VOL_NAME}

if [ ! "${VOL_NAME}" ]; then
	VOL_NAME=${APP_BUNDLE_NAME%.*}
	echo "Defaulting dmg volume name to ${VOL_NAME}"
fi


BG_IMG_NAME=${ARG_BACKGROUND}
VOL_ICON_NAME=${ARG_ICON}

if [ "${ARG_ADD_VERSION}" ]; then
	APP_VERSION=`/usr/libexec/PlistBuddy -c Print:CFBundleShortVersionString ${APP_BUNDLE_NAME}/Contents/Info.plist`
	APP_BUILD_VERSION=`/usr/libexec/PlistBuddy -c Print:CFBundleVersion ${APP_BUNDLE_NAME}/Contents/Info.plist`
	DMG_NAME_SUFFIX=" ${APP_VERSION}.${APP_BUILD_VERSION}"
else
	APP_VERSION=
	APP_BUILD_VERSION=
	DMG_NAME_SUFFIX=
fi

DMG_NAME_TMP="${ARG_DMG_NAME}_tmp.dmg"

if [ "${ARG_DMG_NAME}" ]; then
	DMG_NAME_BASE=${ARG_DMG_NAME}
else
	DMG_NAME_BASE=${APP_BUNDLE_NAME%.*}
fi

DMG_NAME="${DMG_NAME_BASE}${DMG_NAME_SUFFIX}.dmg"

CODESIGN_IDENTITY=${ARG_CODESIGN_ID}

if [ "${CODESIGN_IDENTITY}" ]; then
	echo "*** Signing ${APP_BUNDLE_NAME} with identity '${CODESIGN_IDENTITY}'... "
	codesign -fs "${CODESIGN_IDENTITY}" "${APP_BUNDLE_NAME}"
	echo "done!"
fi

echo -n "*** Copying ${APP_BUNDLE_NAME} to the temporary dir... "
rm -rf "$TMP_DIR"
mkdir "$TMP_DIR"
rm -f "${APP_BUNDLE_NAME}/Contents/MacOS/vmprotect_*.*"
cp -R "${APP_BUNDLE_NAME}" ${TMP_DIR}/
mv "${TMP_DIR}/${APP_BUNDLE_NAME}" "${TMP_DIR}/${ARG_DMG_NAME}.app"
if [ -d "${TMP_DIR}/VMProtect Professional.app" ]; then
  mv "${TMP_DIR}/VMProtect Professional.app" "${TMP_DIR}/VMProtect Pro.app"
fi
cp ./../../../iss/license*.* ${TMP_DIR}/
mv ${TMP_DIR}/license_en.txt "${TMP_DIR}/License Agreement.txt"
mv ${TMP_DIR}/license_ru.txt "${TMP_DIR}/Лицензионное соглашение.txt"
echo "done!"

echo -n "*** Creating temporary dmg disk image..."
rm -f "${TARGET_DIR}/${DMG_NAME_TMP}"
hdiutil create -ov -srcfolder $TMP_DIR -format UDRW -volname "${VOL_NAME}" "${TARGET_DIR}/${DMG_NAME_TMP}"

echo -n "*** Mounting temporary image... "
device=$(hdiutil attach -readwrite -noverify -noautoopen "${TARGET_DIR}/${DMG_NAME_TMP}" | egrep '^/dev/' | sed 1q | awk '{print $1}')
echo "done! (device ${device})"

echo -n "*** Sleeping for 5 seconds..."
sleep 5
echo " done!"

echo "*** Setting style for temporary dmg image..."


if [ "${ARG_BACKGROUND}" ]; then
	echo -n "    * Copying background image... "

	BG_FOLDER="/Volumes/${VOL_NAME}/.background"
	mkdir "${BG_FOLDER}"
	cp "${BG_IMG_NAME}" "${BG_FOLDER}/"

	echo "done!"
	NO_BG=
else
	NO_BG="-- "
fi

if [ "${ARG_ICON}" ]; then
	echo -n "    * Copying volume icon... "

	ICON_FOLDER="/Volumes/${VOL_NAME}"
	cp "${VOL_ICON_NAME}" "${ICON_FOLDER}/.VolumeIcon.icns"

	echo "done!"

	echo -n "    * Setting volume icon... "

	SetFile -c icnC "${ICON_FOLDER}/.VolumeIcon.icns"
	SetFile -a C "${ICON_FOLDER}"

	echo "done!"
fi

if [ "${ARG_COORDS}" ]; then
	ALL_COORDS=(`echo "${ARG_COORDS}" | tr ":" "\n"`)
	APPS_X=${ALL_COORDS[0]}
	APPS_Y=${ALL_COORDS[1]}
	BUNDLE_X=${ALL_COORDS[2]}
	BUNDLE_Y=${ALL_COORDS[3]}
	NO_COORDS=""
else
	NO_COORDS="-- "
fi

if [ "${ARG_SIZE}" ]; then
	WINDOW_SIZE=(`echo "${ARG_SIZE}" | tr ":" "\n"`)
	WINDOW_WIDTH=${WINDOW_SIZE[0]}
	WINDOW_HEIGHT=${WINDOW_SIZE[1]}
else
	WINDOW_WIDTH=640
	WINDOW_HEIGHT=480
fi

WINDOW_LEFT=400
WINDOW_TOP=100

WINDOW_RIGHT=$((${WINDOW_LEFT} + ${WINDOW_WIDTH}))
WINDOW_BOTTOM=$((${WINDOW_TOP} + ${WINDOW_HEIGHT}))

echo -n "    * Executing applescript for further customization... "
APPLESCRIPT="
tell application \"Finder\"
	tell disk \"${VOL_NAME}\"
		open
		-- Setting view options
		set current view of container window to icon view
		set toolbar visible of container window to false
		set statusbar visible of container window to false
		set the bounds of container window to {${WINDOW_LEFT}, ${WINDOW_TOP}, ${WINDOW_RIGHT}, ${WINDOW_BOTTOM}}
		set theViewOptions to the icon view options of container window
		set arrangement of theViewOptions to not arranged
		set icon size of theViewOptions to 128
		-- Settings background
		${NO_BG}set background picture of theViewOptions to file \".background:vmprotect_dmg_1a.png\"
		-- Adding symlink to /Applications
		make new alias file at container window to POSIX file \"/Applications\" with properties {name:\"Applications\"}
		-- Reopening
		close
		open
		-- Rearranging
		${NO_COORDS}set file_list to every item
		${NO_COORDS}repeat with i in file_list
		${NO_COORDS}  if the name of i is \"Applications\" then
		${NO_COORDS}    set the position of i to {${APPS_X}, ${APPS_Y}}
		${NO_COORDS}  else if the name of i ends with \".app\" then
		${NO_COORDS}    set the position of i to {${BUNDLE_X}, ${BUNDLE_Y}}
		${NO_COORDS}  else if the name of i is \"License Agreement.txt\" then
		${NO_COORDS}    set the position of i to {220, 470}
		${NO_COORDS}  else if the name of i is \"Лицензионное соглашение.txt\" then
		${NO_COORDS}    set the position of i to {370, 470}
		${NO_COORDS}  else
		${NO_COORDS}    set the position of i to {1000, 1000}
		${NO_COORDS}  end if
		${NO_COORDS}  set extension hidden of i to true
		${NO_COORDS}end repeat
		-- Updating and sleeping for 5 secs
		update without registering applications
		delay 5
	end tell
end tell
"

echo "$APPLESCRIPT" | osascript
echo "done!"

echo -n "    *  Hiding extensions..."
sudo find /Volumes/"${VOL_NAME}" -name "*.*" -exec SetFile -a E '{}' \;

echo "done!"

echo "*** Converting temporary dmg image in compressed readonly final image... "
echo "    * Changing mode and syncing..."
sudo chmod -Rf go-w /Volumes/"${VOL_NAME}"
sync
sync
echo "    * Detaching ${device}..."
hdiutil detach -force ${device}
rm -f "${TARGET_DIR}/${DMG_NAME}"
echo "    * Converting..."
hdiutil convert "${TARGET_DIR}/${DMG_NAME_TMP}" -format UDBZ -imagekey zlib-level=9 -o "${TARGET_DIR}/${DMG_NAME}"
echo "done!"

#echo -n "*** Removing temporary image... "
#rm -f "${TARGET_DIR}/${DMG_NAME_TMP}"
#echo "done!"

echo -n "*** Cleaning up temp folder... "
rm -rf $TMP_DIR
echo "done!"

echo "
*** Everything done. DMG disk image is ready for distribution.
"
