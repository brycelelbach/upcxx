########################################################################
#
#  Create a safely-encoded PostScript string.
#


sub psString ($) {
    local $_ = shift;
    s/[()\\]/\\$&/g;
    return "($_)";
}


########################################################################


1;
