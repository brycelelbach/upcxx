package Spans;

require Exporter;
@EXPORT = qw();


########################################################################


sub new ($) {
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $self = { 'pending' => [],
		 'complete' => [] };
    
    bless $self, $class;
}


########################################################################


sub begin ($$) {
    my ($self, $event) = @_;
    $event->{desc} =~ s/^begin //;
    push @{$self->pending}, $event;
}


sub end ($$) {
    my ($self, $end) = @_;
    $end->{desc} =~ s/^end //;
    my $begin = pop @{$self->pending};
    
    my $finished = $self->merge($begin, $end);
    push @{$self->complete}, $finished;
}


########################################################################


sub complete ($) {
    return shift->{complete};
}


sub pending ($) {
    return shift->{pending};
}


########################################################################


sub print ($$) {
    my ($self, $out) = @_;
    die 'unfinished ', $self->things if @{$self->pending};

    foreach (reverse @{$self->complete}) {
	$_->print($out);
    }
}


########################################################################


1;
