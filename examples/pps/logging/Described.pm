package Described;

use PostScript;


########################################################################


sub desc ($) {
    return shift->{desc};
}


sub psDesc ($) {
    return psString shift->{'desc'};
}


########################################################################


1;
