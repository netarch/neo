import ipaddress
from collections import deque
from pathlib import Path

import matplotlib.pyplot as plt
import networkx as nx


class NetSynthesizer:
    def __init__(self, linkfile):
        self.G = dict()
        self.subnets = dict()
        self.parent_map = dict()
        self.node_to_interfaces = dict()
        self.interface_to_node = dict()
        self.interface_links = dict()
        self.links = []
        self.rules = dict()
        self.topo_name = Path(linkfile).stem

        links = read_dsv(linkfile)
        subnet = 0
        # print(links)
        for link in links:
            if link[0] == link[1]:
                continue
            if link[0] not in self.G:
                self.G[link[0]] = set()
                self.node_to_interfaces[link[0]] = []
            if link[1] not in self.G:
                self.G[link[1]] = set()
                self.node_to_interfaces[link[1]] = []
            self.G[link[0]].add(link[1])
            self.G[link[1]].add(link[0])

            if (link[0], link[1]) not in self.subnets and (
                link[1],
                link[0],
            ) not in self.subnets:
                subnet_str = str(ipaddress.IPv4Address(subnet))
                ip_1 = ipaddress.IPv4Address(subnet + 1)
                ip_2 = ipaddress.IPv4Address(subnet + 2)
                if1 = "eth" + str((len(self.node_to_interfaces[link[0]])))
                if2 = "eth" + str((len(self.node_to_interfaces[link[1]])))
                self.subnets[(link[0], link[1])] = ipaddress.ip_network(
                    subnet_str + "/30"
                )
                self.node_to_interfaces[link[0]].append((if1, ip_1))
                self.interface_to_node[ip_1] = link[0]
                self.node_to_interfaces[link[1]].append((if2, ip_2))
                self.interface_to_node[ip_2] = link[1]
                if ip_1 not in self.interface_links:
                    self.interface_links[ip_1] = ip_2
                    self.interface_links[ip_2] = ip_1
                self.links.append((link[0], if1, link[1], if2))
                subnet += 4

        for n in self.node_to_interfaces:
            self.rules[n] = self.synthesize_rules(n)
        # print(self.interface_links)
        # print(self.node_to_interfaces)
        # print(self.interface_to_node)
        # print(self.rules)
        # print(self.subnets)
        # print(self.node_to_interfaces)
        # print(self.links)

    def visualize(self):
        G = nx.Graph()
        for n in self.node_to_interfaces:
            G.add_node(n)
        for link in self.links:
            G.add_edge(link[0], link[2], headlabel=link[1], taillabel=link[3])

        pos = nx.spring_layout(G)
        nx.draw_networkx(G, pos)
        head_labels = nx.get_edge_attributes(G, "headlabel")
        tail_labels = nx.get_edge_attributes(G, "taillabel")

        nx.draw_networkx_edge_labels(
            G, pos, label_pos=0.25, edge_labels=head_labels, rotate=False
        )
        nx.draw_networkx_edge_labels(
            G, pos, label_pos=0.75, edge_labels=tail_labels, rotate=False
        )

        plt.axis("off")
        plt.savefig(self.topo_name + ".png")

    def synthesize_rules(self, src):
        parent = BFSParent(self.G, src)
        subnet_to_parent = {}
        parent_to_subnet = {}
        for edge in self.subnets:
            if edge[0] != src and edge[1] != src and parent[edge[0]] is not None:
                subnet_to_parent[self.subnets[edge]] = parent[edge[0]]
        for subnet in subnet_to_parent:
            if subnet_to_parent[subnet] not in parent_to_subnet:
                parent_to_subnet[subnet_to_parent[subnet]] = []
            parent_to_subnet[subnet_to_parent[subnet]].append(subnet)
        for p in parent_to_subnet:
            parent_to_subnet[p] = list(
                ipaddress.collapse_addresses(parent_to_subnet[p])
            )

        rules = []
        for p in parent_to_subnet:
            di = self.get_dst_interface(src, p)
            for s in parent_to_subnet[p]:
                rules.append((s, di))
        return rules

    def get_dst_interface(self, src, dst):
        for i in self.node_to_interfaces[src]:
            di = self.interface_links[i[1]]
            dn = self.interface_to_node[di]
            if dn == dst:
                return di

    def leaves(self):
        return bfs_leaves(self.G, list(self.G)[0])

    def get_firewall(self, n):
        leaves = self.leaves()
        if n not in leaves:
            return -1
        return list(self.G[n])[0]

    def get_itf_to_leaf(self, n):
        leaves = self.leaves()
        for i in self.node_to_interfaces[n]:
            di = self.interface_links[i[1]]
            dn = self.interface_to_node[di]
            if dn in leaves:
                return i[0]


# class FileParser:
#     """
#     nodefile : the nodes filename. The file must be space-separated, each line describing a single node.
#                 node name at the first column, followed by the interfaces
#
#     rulefile : the rules filename. The file must be space-separated, each line descriing a single rule.
#                 node name at the first column, destination subnet in the second column, destination address at the third column.
#     """
#
#     def __init__(self, nodefile=None, rulefile=None, linkfile=None):
#         self.nodes = read_dsv(nodefile)
#         self.rules = read_dsv(rulefile)
#         self.ntoidict = dict()
#         self.itondict = dict()
#
#         for line in self.nodes:
#             self.ntoidict[line[0]] = []
#             for interface in line[1:]:
#                 self.ntoidict[line[0]].append(interface)
#                 self.itondict[interface] = line[0]
#
#     def visualize(self):
#         G = nx.Graph()
#         for n in self.nodes:
#             G.add_node(n[0])
#         for r in self.rules:
#             srcinterface = self.findMatchingInterface(r[0], r[1])
#             dstnode = self.itondict[r[2]]
#             G.add_edge(r[0], dstnode, headlabel=srcinterface, taillabel=r[2])
#
#         pos = nx.spring_layout(G)
#
#         nx.draw_networkx(G, pos)
#         head_labels = nx.get_edge_attributes(G, "headlabel")
#         tail_labels = nx.get_edge_attributes(G, "taillabel")
#
#         nx.draw_networkx_edge_labels(
#             G, pos, label_pos=0.25, edge_labels=head_labels, rotate=False
#         )
#         nx.draw_networkx_edge_labels(
#             G, pos, label_pos=0.75, edge_labels=tail_labels, rotate=False
#         )
#
#         plt.axis("off")
#         plt.savefig("topo.png")
#
#     def findMatchingInterface(self, node, subnet):
#         for interface in self.ntoidict[node]:
#             if ipaddress.ip_address(interface) in ipaddress.ip_network(subnet):
#                 return interface


def BFSParent(G, s):
    parent = {}
    q = deque()
    for n in G:
        parent[n] = None
    if s not in parent:
        raise Exception("BFSParent: key not in graph")
    parent[s] = s
    for neighbor in G[s]:
        parent[neighbor] = neighbor
        q.append(neighbor)

    while len(q) > 0:
        n = q.popleft()
        for neighbor in G[n]:
            if parent[neighbor] is None:
                parent[neighbor] = parent[n]
                q.append(neighbor)
    return parent


def bfs_leaves(G, s):
    leaves = []
    q = deque()
    q.append(s)
    visited = {}
    for n in G:
        visited[n] = False

    while len(q) > 0:
        n = q.popleft()
        visited[n] = True
        if len(G[n]) == 1:
            leaves.append(n)
        for neighbor in G[n]:
            if not visited[neighbor]:
                q.append(neighbor)
    return leaves


def bfs_is_connected(G, src, dst) -> bool:
    q = deque()
    q.append(src)
    visited = dict()
    for n in G:
        visited[n] = False

    while len(q) > 0:
        n = q.popleft()
        if n == dst:
            return True
        visited[n] = True
        for neighbor in G[n]:
            if not visited[neighbor]:
                q.append(neighbor)

    return False


def read_dsv(filename):
    if filename is None:
        return []
    f = open(filename, "r")
    lines = f.readlines()
    f.close()
    lines = split_lines(lines)
    return lines


def remove_comments(lines):
    ret = []
    for line in lines:
        if line[0] != "#":
            ret.append(line)
    return ret


def split_lines(lines):
    ret = []
    for line in lines:
        ret.append(line.split())
    return ret


"""
def visualize(links):
    nodes=set()
    G = graphviz.Graph()
    for link in links:
        if link[0] not in nodes:
            nodes.add(link[0])
            G.node(link[0],link[0])
        if link[1] not in nodes:
            nodes.add(link[1])
            G.node(link[1], link[1])
        G.edge(link[0], link[1])
    G.format='png'
    G.render('Graph', view=True)
"""
