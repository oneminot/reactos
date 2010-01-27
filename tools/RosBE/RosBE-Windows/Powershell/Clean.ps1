#
# PROJECT:     RosBE - ReactOS Build Environment for Windows
# LICENSE:     GNU General Public License v2. (see LICENSE.txt)
# FILE:        Root/Clean.ps1
# PURPOSE:     Clean the ReactOS source directory.
# COPYRIGHT:   Copyright 2010 Daniel Reimer <reimer.daniel@freenet.de>
#

$host.ui.RawUI.WindowTitle = "Cleaning..."

function remlog {
    # Check if we have any logs to clean, if so, clean them.
    if (Test-Path "$_ROSBE_LOGDIR") {
        "Cleaning build logs..."
        $null = (Remove-Item -path "$_ROSBE_LOGDIR\*.txt" -force)
        "Done cleaning build logs."
    } else {
        "ERROR: There are no logs to clean."
    }
}

function rembin {
    # Check if we have any binaries to clean, if so, clean them.

    # Apply modified obj and out paths for deletion.

    if ("$_ROSBE_OBJPATH" -eq "") {
        $OBJCLEANPATH = "obj-$ENV:ROS_ARCH"
    } else {
        $OBJCLEANPATH = "$_ROSBE_OBJPATH"
    }

    if ("$_ROSBE_OUTPATH" -eq "") {
        $OUTCLEANPATH = "output-$ENV:ROS_ARCH"
    } else {
        $OUTCLEANPATH = "$_ROSBE_OUTPATH"
    }

    if ("$ENV:ROS_ARCH" -eq "i386") {
        $MAKEFILE = "makefile.auto"
    } else {
        $MAKEFILE = "makefile-$ENV:ROS_ARCH.auto"
    }

    if (Test-Path "$MAKEFILE") {
        $null = (Remove-Item "$MAKEFILE" -force)
    }

    if (Test-Path "$OBJCLEANPATH") {
        "Cleaning ReactOS $ENV:ROS_ARCH source directory..."
        if (Test-Path "$OBJCLEANPATH") {
            $null = (Remove-Item "$OBJCLEANPATH" -recurse -force)
        }
        if (Test-Path "$OUTCLEANPATH") {
            $null = (Remove-Item "$OUTCLEANPATH" -recurse -force)
        }
        "Done cleaning ReactOS $ENV:ROS_ARCH source directory."
    } else {
        "ERROR: There is no $ENV:ROS_ARCH compiler output to clean."
    }

    if (Test-Path "reactos") {
        $null = (Remove-Item "reactos" -recurse -force)
    }
}

function end {
    $host.ui.RawUI.WindowTitle = "ReactOS Build Environment $_ROSBE_VERSION"
    exit
}

if ("$args" -eq "") {
    rembin
}
elseif ("$args" -eq "logs") {
    remlog
}
elseif ("$args" -eq "all") {
    rembin
    remlog
}
elseif ("$args" -ne "") {
    $argindex = 0
    while ( "$($args[$argindex])" -ne "") {
        $cl = "$($args[$argindex])" + "_clean"
        make $cl
        $argindex += 1
    }
    remove-variable cl
}
end
