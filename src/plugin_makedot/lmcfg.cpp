/*!
lmcfg.cpp

Implements functions for manipulating the control flow graph of a
function using the Boost Graph Library.
*/
#define USE_DANGEROUS_FUNCTIONS

#include <iostream>
#include <cstdio>
#include <vector>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <funcs.hpp>
#include <xref.hpp>
#include <fstream>
#include <stack>
#include <map>
#include <errno.h>
#include "function_analyzer.h"
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/depth_first_search.hpp>
#undef USE_DANGEROUS_FUNCTIONS

#include "lmcfg.h"

extern int errno;

using namespace std;

LMCFG::LMCFG()
{
    pFA = NULL;
}

LMCFG::~LMCFG()
{
}

int LMCFG::GenGraph(function_analyzer *pFuncAnalyzer)
{
    // Create file analyzer
    pFA = pFuncAnalyzer;
    vertex_idx = get(vertex_index1, g);

    AddVertices();
    AddEdges();
    //msg("lmcfg: %d vertices %d edges added.\n", num_vertices(g), num_edges(g));
}

/*!
Adds all vertices to the Boost Graph g.
*/
int LMCFG::AddVertices()
{
    int num_nodes;
    int i;

    num_nodes = pFA->get_num_nodes();
    for (i = 1; i <= num_nodes; i++)
    {
        AddVertex(pFA->get_node(i));
    }

    return 0;
}

/*!
Add a single vertex to the Boost Graph g.
*/
int LMCFG::AddVertex(ea_t nodeaddr)
{
    Vertex v;
    v = add_vertex(g);
    vertex_idx[v] = nodeaddr;
    //msg("Adding vertex %a\n", vertex_idx[v]);

    return 0;
}

/*!
Finds vertex v in Boost Graph g for given addr.
*/
Vertex LMCFG::FindVertex(ea_t addr)
{
    if (num_vertices(g) == 0)
        return graph_traits<Graph>::null_vertex();

    std::pair<vertex_iter, vertex_iter> vp;
    for (vp = vertices(g); vp.first != vp.second; ++vp.first)
    {
        if (vertex_idx[*vp.first] == addr)
        {
            // Found vertex
            return *vp.first;
        }
    }

    // Cannot find vertex
    return graph_traits<Graph>::null_vertex();
}

/*
Finds edge from u to v in Boost Graph g for given addr.

Returns true if edge pointed to by e is found from u to v.
Otherwise, e is undefined.
*/
bool LMCFG::FindEdge(Vertex u, Vertex v, Edge *e)
{
    Vertex w;

    edge_iter ei, ei_end;
    tie(ei, ei_end) = edges(g);
    for (; ei != ei_end; ++ei)
    {
        w = source(*ei, g);
        if (w != u) continue;

        w = target(*ei, g);
        if (w == v)
        {
            *e = *ei;
            return true;
        }
    }

    return false;
}

/*!
Adds all edges to the Boost Graph g.
*/
int LMCFG::AddEdges()
{
    int num_edges;
    int i;
    int res;
    ea_t src, dst;
    Vertex srcV, dstV;

    num_edges = pFA->get_num_edges();
    for (i = 1; i <= num_edges; i++)
    {
        src = pFA->get_node(pFA->get_edge_src(i));
        srcV = FindVertex(src);
        if ( srcV == graph_traits<Graph>::null_vertex()) continue;

        dst = pFA->get_node(pFA->get_edge_dst(i));
        dstV = FindVertex(dst);
        if ( dstV == graph_traits<Graph>::null_vertex() ) continue;

        // msg("Adding edge %a => %a\n", vertex_idx[srcV], vertex_idx[dstV]);
        add_edge(srcV, dstV, g);
    }

    // Need to add implicit edges. Function Analyzer doesn't do it.
    // These are the edges from one instruction to the next, where
    // the next instruction is the start of a new node.
    // Branching/Return instructions are ignored here.
    bool found;
    Edge e;
    ea_t ea;
    char disasm_buf[FA_DISASM_BUFLEN];
    for (ea = pFA->first_ea(); ea != BADADDR; ea = pFA->next_ea(ea))
    {
        memset(disasm_buf, 0, sizeof(disasm_buf));
        pFA->disasm(ea, disasm_buf);

        if (strnicmp(disasm_buf, "ret", 3) == 0)
            continue;

        // msg("disasm_buf: %s\n", disasm_buf);
        if (get_first_fcref_from(ea) != BADADDR && disasm_buf[0] == 'j')
            continue;

        if (pFA->is_node(pFA->next_ea(ea)))
        {
            for (src = ea; src != BADADDR; src = prev_visea(src))
                if (pFA->is_node(src))
                    break;

            dst = pFA->next_ea(ea);

            // Get Boost Vertex
            srcV = FindVertex(src);
            if ( srcV == graph_traits<Graph>::null_vertex() ) continue;
            dstV = FindVertex(dst);
            if ( dstV == graph_traits<Graph>::null_vertex() ) continue;

            // Add edge if doesn't exist
            tie(e, found) = edge(srcV, dstV, g);
            if (found == false)
            {
                // msg("Adding edge %a => %a (implicit)\n", vertex_idx[srcV], vertex_idx[dstV]);
                add_edge(srcV, dstV, g);
            }
        }
    }

    return 0;
}

int LMCFG::ShowGraph()
{
    char path[1024];

    // Generate a random file name in the
    // temp directory to store our graph.
    qtmpnam(path, 1024);
    path[qstrlen(path)-4] = '_';
    qstrncpy(&path[qstrlen(path)-3], pFA->get_name(), sizeof(path) - qstrlen(path));
    qstrncpy(&path[qstrlen(path)], ".dot", sizeof(path) - qstrlen(path));
    msg("Writing graph at %s: %d,%d nodes %d,%d edges\n", path,
        pFA->get_num_nodes(), num_vertices(g),
        pFA->get_num_edges(), num_edges(g));

    std::ofstream ofs(path);
    write_graphviz(ofs, g);

    return 0;
}

void LMCFG::PrintVertices()
{
    std::pair<vertex_iter, vertex_iter> vp;
    msg("lmcfg: Printing vertices...\n");
    for (vp = vertices(g); vp.first != vp.second; ++vp.first)
    {
        msg("lmcfg: * %a\n", vertex_idx[*vp.first]);
    }
}

/*
void LMCFG::PrintIdoms()
{
    msg("lmcfg: Printing idoms...\n");

    // Print out immediate dominators
    Vertex v;
    for (int i = 0, num_vert = num_vertices(g); i < num_vert; ++i)
    {
        msg("[%d]", indexMap[i]);
        v = domTreePredMap[indexMap[i]];
        if ( v != graph_traits<Graph>::null_vertex() )
            msg("lmcfg: (%a,%d) ", vertex_idx[v], GetVertexIdx(v, i));
        else
            msg("lmcfg: (%a,%d) ", BADADDR, GetVertexIdx(v, i));
    }
    msg("\n");
}
*/
/*!
Computes dominator tree.
*/
/*
int LMCFG::ComputeDomTree()
{
    indexMap = IndexMap(get(vertex_index, g));

    // Lengauer-Tarjan dominator tree algorithm
    domTreePredVector = std::vector<Vertex>(num_vertices(g), graph_traits<Graph>::null_vertex());
    domTreePredMap = make_iterator_property_map(domTreePredVector.begin(), indexMap);
    lengauer_tarjan_dominator_tree(g, vertex(0, g), domTreePredMap);
}
*/
/*!
Gets the vertex index, beginning the search from starti.

Specify starti such that vertex v is expected to have an index
smaller than and near to starti.
*/
int LMCFG::GetVertexIdx(Vertex v, int starti)
{
    int num_vert = num_vertices(g);
    int vert_cnt;
    for (vert_cnt = 0; vert_cnt != num_vert; ++vert_cnt)
    {
        if (vertex(starti, g) == v)
            return starti;

        // Decrement and wrap around if necessary
        if (--starti < 0) starti = num_vert-1;
    }

    return -1;
}
/*
int LMCFG::ComputeLoops()
{
    //msg("lmcfg: Computing loops...\n");
    ComputeDomTree();

    Vertex domv;
    int domi;
    for (int i = 0, num_vert = num_vertices(g); i < num_vert; ++i)
    {
        domv = domTreePredMap[indexMap[i]];
        while ( domv != graph_traits<Graph>::null_vertex() )
        {
            Edge e;
            bool found;

            // Get index of dominator
            domi = GetVertexIdx(domv, i);

            tie(e, found) = edge(vertex(indexMap[i], g), domv, g);
            if (found == true)
            {
                //msg("lmcfg: \tBackedge from %d to %d\n", i, domi);
            }

            domv = domTreePredMap[indexMap[domi]];
        }
    }
    return 0;
}
*/
using namespace boost;

template < typename Graph, typename Loops > void
LMCFG::find_loops(typename graph_traits < Graph >::vertex_descriptor entry,
                  const Graph & g,
                  Loops & loops)    // A container of sets of vertices
{
    function_requires < BidirectionalGraphConcept < Graph > >();
    typedef typename graph_traits < Graph >::edge_descriptor Edge;
    typedef typename graph_traits < Graph >::vertex_descriptor Vertex;
    std::vector < Edge > back_edges;
    std::vector < default_color_type > color_map(num_vertices(g));
    depth_first_visit(g, entry,
                      make_back_edge_recorder(std::back_inserter(back_edges)),
                      make_iterator_property_map(color_map.begin(),
                                                 get(vertex_index, g), color_map[0]));

    //std::vector < Edge >::size_t i ;
    std::size_t i;
    for (i = 0; i < back_edges.size(); ++i) {
        typename Loops::value_type x;
        loops.push_back(x);
        compute_loop_extent(back_edges[i], g, loops.back());
    }
}

template < typename Graph, typename Set > void
LMCFG::compute_loop_extent(typename graph_traits <
                           Graph >::edge_descriptor back_edge, const Graph & g,
                           Set & loop_set)
{
    function_requires < BidirectionalGraphConcept < Graph > >();
    typedef typename graph_traits < Graph >::vertex_descriptor Vertex;
    typedef color_traits < default_color_type > Color;

    Vertex loop_head, loop_tail;
    loop_tail = source(back_edge, g);
    loop_head = target(back_edge, g);

    std::vector < default_color_type >
    reachable_from_head(num_vertices(g), Color::white());
    default_color_type c;
    depth_first_visit(g, loop_head, default_dfs_visitor(),
                      make_iterator_property_map(reachable_from_head.begin(),
                                                 get(vertex_index, g), c));

    std::vector < default_color_type > reachable_to_tail(num_vertices(g));
    reverse_graph < Graph > reverse_g(g);
    depth_first_visit(reverse_g, loop_tail, default_dfs_visitor(),
                      make_iterator_property_map(reachable_to_tail.begin(),
                                                 get(vertex_index, g), c));

    typename graph_traits < Graph >::vertex_iterator vi, vi_end;
    for (tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
        if (reachable_from_head[*vi] != Color::white()
                && reachable_to_tail[*vi] != Color::white())
            loop_set.insert(*vi);
}


