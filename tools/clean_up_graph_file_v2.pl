#!/usr/bin/perl -w

use strict;

die "Usage: $0 dir_contain_gdl_file" unless ($#ARGV == 0);

opendir GDL, "$ARGV[0]" or die "Cannot open dir $ARGV[0]";
my @allfiles = readdir GDL;
closedir GDL;

my @all_gdl = ();

my $dir_name = $ARGV[0];
my @lst = split /\/|\\/, $dir_name;
my $input_name = $lst[-1];
print $input_name;


my $output = $input_name.".graphs.clean"; #all_graphs.clean";

open OUT, "> $output" or die "Cannot open $output\n";

foreach my $file (@allfiles) 
{
	next if ($file !~ /\.gdl$/);
	my $fullpath = $ARGV[0]."/".$file;
	push @all_gdl, $fullpath;
	
}  
my @tmp = sort @all_gdl;
@all_gdl = @tmp;

#print join("\n", @all_gdl);

# first output the number of graphs
print OUT $#all_gdl+1, "\n";

foreach my $gdl (@all_gdl) {
	my %title_to_label = ();
	my %edge = ();
	my $title;
	my $tmp;
	my %all_nodes; 
	
	if (!open GDLF, "< $gdl") {
		print STDERR "Error open $gdl\n";
		next;
	} 
	
	my $edge_num = 0;
	while (<GDLF>) {
		chomp;
		if (/^title:/) {
			($tmp, $title) = split /:\s+/;
			next;		
		}
		
		if (/^\s+node:\s+{\s+title:\s+(.*?)\s+}/)
		{
			my $title = $1;
		 	my $label = $1;
			
			$label = "sub_" if ($label =~ /^sub_/i);
			$label = "nullsub_" if ($label =~ /^nullsub_/i);
			
			$title_to_label{$title} = $label;
			$all_nodes{$title}  = 1; 
		}
		elsif (/^\s+node:\s+{\s+label:\s+(.*?)\s+.*?title:\s+(.*?)\s+/)
		{
			my $title = $2;
		 	my $label = $1;
			
			$label = "sub_" if ($label =~ /^sub_/i);
			$label = "nullsub_" if ($label =~ /^nullsub_/i);
			
			$title_to_label{$title} = $label;
			$all_nodes{$title}  = 1; 
		}
		elsif (/^node:\s+{\s+title:\s+"(.*?)"\s+.*?label:\s+"(.*?)"\s+/ || 
			   /^node:\s+{\s+title:\s+"(.*?)"\s+.*?label:\s+"(.*?)$/) 
		{
		 	my $title = $1;
		 	my $label = $2;
			
			$label = "sub_" if ($label =~ /^sub_/i);
			$label = "nullsub_" if ($label =~ /^nullsub_/i);
			
			$title_to_label{$title} = $label;
			$all_nodes{$title}  = 1; 
		}
		elsif (/edge:\s+{\s*sourcename:\s+"(.*?)"\s+targetname:\s+"(.*?)"\s+}/ ||
				/edge:\s+{.*?sourcename:\s+(.*?)\s+targetname:\s+(.*?)\s+}/) 
		{
			my $src = $1;
			my $dst = $2;
			push @{$edge{$src}}, $dst;
			$edge_num += 1;
			
		}
		
		
	} # END <GDLF>
	my @all_nodes_lst = keys %all_nodes;
	my @all_edges_lst = keys %edge;
	print $gdl, ' node_num: ', $#all_nodes_lst+1, ' edge_num: ', $edge_num, "\n";
	
	# output info in the format 
	# title of the graph
	# number of vertices
	# \\ then for every vertex
	# label \\ {if you could also delete the numbers after the subs when printing them it would be great!}
	# number of edges going out (say m)
	# destination node 1
	# ..........................
	# destination node m
	
	# output title
	my @fn = split /\//, $gdl;
	
	print OUT $fn[$#fn], "\n";
	
	# output vertice number
	my @all_vertices = keys %all_nodes;
	print OUT $#all_vertices+1, "\n";
	
	# print out all vertices and edges
	foreach my $src_title (sort {$a<=>$b} keys %edge) {
		my @dst_title = @{$edge{$src_title}};
		my $label = $title_to_label{$src_title};
		
		# output number and label
		
		print OUT $src_title, "\n", $label, "\n";	
		

		# output out degree
		print OUT $#dst_title+1, "\n";
		
		foreach my $dst_t (sort {$a<=>$b} @dst_title) {
			print OUT $dst_t, "\n";
		}
		#print OUT "\n";
		
		delete $all_nodes{$src_title};
		 
	} 
	
	# output remaining nodes
	foreach my $remain_vertex (keys %all_nodes) {
		# print label
		my $label = $title_to_label{$remain_vertex};
		
		# output number and label
	
		print OUT $remain_vertex, "\n", $label, "\n";	
		
		# out degree is 0
		print OUT "0\n";
		#print OUT "\n";	
	} # end foreach remain vertex
	

	#print OUT "\n";
}



close OUT;
