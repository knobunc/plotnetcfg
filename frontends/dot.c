/*
 * This file is a part of plotnetcfg, a tool to visualize network config.
 * Copyright (C) 2014 Red Hat, Inc. -- Jiri Benc <jbenc@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <sys/socket.h>
#include <time.h>
#include "../frontend.h"
#include "../handler.h"
#include "../if.h"
#include "../label.h"
#include "../netns.h"
#include "../utils.h"
#include "../version.h"
#include "dot.h"

static void output_label(FILE *f, struct label *list)
{
	struct label *ptr;

	for (ptr = list; ptr; ptr = ptr->next)
		fprintf(f, "\\n%s", ptr->text);
}

static void output_label_properties(FILE *f, struct label_property *list, unsigned int prop_mask)
{
	struct label_property *ptr;

	for (ptr = list; ptr; ptr = ptr->next)
		if (label_prop_match_mask(ptr->type, prop_mask))
			fprintf(f, "\\n%s: %s", ptr->key, ptr->value);
}

static void output_addresses(FILE *f, struct if_addr_entry *list)
{
	struct if_addr_entry *ptr;

	for (ptr = list; ptr; ptr = ptr->next) {
		fprintf(f, "\\n%s", ptr->addr.formatted);
		if (ptr->peer.formatted)
			fprintf(f, " peer %s", ptr->peer.formatted);
	}
}

static void output_mtu(FILE *f, struct if_entry *ptr)
{
	if (ptr->mtu && ptr->mtu != 1500 && !(ptr->flags & IF_LOOPBACK))
		fprintf(f, "\\nMTU %d", ptr->mtu);
}

static void output_ifaces_pass1(FILE *f, struct if_entry *list, unsigned int prop_mask)
{
	struct if_entry *ptr;

	for (ptr = list; ptr; ptr = ptr->next) {
		fprintf(f, "\"%s\" [label=\"%s", ifid(ptr), ptr->if_name);
		if (ptr->driver)
			fprintf(f, " (%s)", ptr->driver);
		output_label_properties(f, ptr->prop, prop_mask);
		output_mtu(f, ptr);
		if (label_prop_match_mask(IF_PROP_CONFIG, prop_mask))
			output_addresses(f, ptr->addr);
		fprintf(f, "\"");

		if (label_prop_match_mask(IF_PROP_STATE, prop_mask)) {
			if (ptr->flags & IF_INTERNAL)
				fprintf(f, ",style=dotted");
			else if (!(ptr->flags & IF_UP))
				fprintf(f, ",style=filled,fillcolor=\"grey\"");
			else if (!(ptr->flags & IF_HAS_LINK))
				fprintf(f, ",style=filled,fillcolor=\"pink\"");
			else
				fprintf(f, ",style=filled,fillcolor=\"darkolivegreen1\"");
		}
		if (ptr->warnings)
			fprintf(f, ",color=\"red\"");
		fprintf(f, "]\n");
	}
}

static void output_ifaces_pass2(FILE *f, struct if_entry *list)
{
	struct if_entry *ptr;

	for (ptr = list; ptr; ptr = ptr->next) {
		if (ptr->master) {
			fprintf(f, "\"%s\" -> ", ifid(ptr));
			fprintf(f, "\"%s\" [style=%s", ifid(ptr->master),
			       ptr->flags & IF_PASSIVE_SLAVE ? "dashed" : "solid");
			if (ptr->edge_label && !ptr->link)
				fprintf(f, ",label=\"%s\"", ptr->edge_label);
			fprintf(f, "]\n");
		}
		if (ptr->physfn) {
			fprintf(f, "\"%s\" -> ", ifid(ptr));
			fprintf(f, "\"%s\" [style=dotted,taillabel=\"PF\"]\n", ifid(ptr->physfn));
		}
		if (ptr->link) {
			fprintf(f, "\"%s\" -> ", ifid(ptr->link));
			fprintf(f, "\"%s\" [style=%s", ifid(ptr),
				ptr->flags & IF_LINK_WEAK ? "dashed" : "solid");
			if (ptr->edge_label)
				fprintf(f, ",label=\"%s\"", ptr->edge_label);
			fprintf(f, "]\n");
		}
		if (ptr->peer && (size_t) ptr > (size_t) ptr->peer) {
			fprintf(f, "\"%s\" -> ", ifid(ptr));
			fprintf(f, "\"%s\" [dir=none]\n", ifid(ptr->peer));
		}
	}
}

static void output_warnings(FILE *f, struct netns_entry *root)
{
	struct netns_entry *ns;
	int was_label = 0;

	for (ns = root; ns; ns = ns->next) {
		if (ns->warnings) {
			if (!was_label)
				fprintf(f, "label=\"");
			was_label = 1;
			output_label(f, ns->warnings);
		}
	}
	if (was_label) {
		fprintf(f, "\"\n");
		fprintf(f, "fontcolor=\"red\"\n");
	}
}

static void dot_output(FILE *f, struct netns_entry *root, struct output_entry *output_entry)
{
	struct netns_entry *ns;
	time_t cur;

	time(&cur);
	fprintf(f, "// generated by plotnetcfg " VERSION " on %s", ctime(&cur));
	fprintf(f, "digraph {\nnode [shape=box]\n");
	for (ns = root; ns; ns = ns->next) {
		if (ns->name) {
			fprintf(f, "subgraph \"cluster/%s\" {\n", nsid(ns));
			fprintf(f, "label=\"%s\"\n", ns->name);
			fprintf(f, "fontcolor=\"black\"\n");
		}
		output_ifaces_pass1(f, ns->ifaces, output_entry->print_mask);
		if (ns->name)
			fprintf(f, "}\n");
	}
	for (ns = root; ns; ns = ns->next) {
		output_ifaces_pass2(f, ns->ifaces);
	}
	output_warnings(f, root);
	fprintf(f, "}\n");
}

static struct frontend fe_dot = {
	.format = "dot",
	.desc = "dot language (suitable for graphviz)",
	.output = dot_output,
};

void frontend_dot_register(void)
{
	frontend_register(&fe_dot);
}
