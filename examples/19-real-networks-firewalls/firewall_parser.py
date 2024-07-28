import csv
import random

class FirewallParser:
    def __init__(self, filename, seed_val):
        f=open(filename, 'r')
        lines=f.readlines()
        f.close()
    
        self.data=[]
        self.port_count_stat=dict()
        self.port_acl_stat=dict()
        self.port_minmax=dict()
        random.seed(seed_val)
        
        for i, l in enumerate(lines):
            if i==0:
                continue
            llist=l.split(',')
            self.add_row(Row(i+1, llist))
            

    def add_row(self,row):
        self.data.append(row)
        
        if row.dport==0:
            return
        if row.action not in ['allow', 'drop', 'deny']:
            return
            

        if row.dport not in self.port_count_stat:
            self.port_count_stat[row.dport]={'allow':0, 'drop':0, 'deny':0}
            self.port_acl_stat[row.dport]={'allow':set(), 'drop':set(), 'deny':set()}
            self.port_minmax[row.dport]={'allow':[-1,-1], 'drop':[-1,-1], 'deny':[-1,-1]}

        if row.action=='allow' or row.action=='deny' or row.action=='drop':
            self.port_count_stat[row.dport][row.action]+=1
            self.port_acl_stat[row.dport][row.action].add(row.sport)

            if self.port_minmax[row.dport][row.action][0] == -1 or self.port_minmax[row.dport][row.action][0]>row.sport:
                self.port_minmax[row.dport][row.action][0]=row.sport

            if self.port_minmax[row.dport][row.action][1] == -1 or self.port_minmax[row.dport][row.action][1]<row.sport:
                self.port_minmax[row.dport][row.action][1]=row.sport



    def dump_port_stat(self):
        for p in self.port_count_stat:
            print("{} : {}".format(p, self.port_count_stat[p]))

    def dump_acl_stat(self):
        for p in self.port_acl_stat:
            print("{} : {}".format(p, self.port_acl_stat[p]))

    def dump_minmax(self):
        for p in self.port_minmax:
            print("{} : {}".format(p, self.port_minmax[p]))

    def get_ranges(self, p):
        action_list=[]
        ret=[]
        for action in self.port_acl_stat[p]:
            for port in self.port_acl_stat[p][action]:
                action_list.append((port, action))
        action_list.sort(key=lambda x:x[0])

        init_idx=0
        last_seen=0
        last_action=None


        for entry in action_list:
            if last_action==None:
                last_action=entry[1]

            elif last_action!=entry[1]:
                ret.append((init_idx, entry[0]-1, last_action))
                init_idx=entry[0]
                last_action=entry[1]

        ret.append((init_idx, 65535, last_action))

        return ret
    def generate_rules(self, itf):
        rules='*filter\n:INPUT DROP [0:0]\n:FORWARD DROP [0:0]\n:OUTPUT ACCEPT [0:0]\n'
        for dport in self.port_acl_stat:
            ranges=self.get_ranges(dport)
            for r in ranges:
                action=""
                if r[2]=='allow':
                    action='ACCEPT'
                if r[2]=='drop':
                    action='DROP'
                if r[2]=='deny':
                    action='REJECT'
                    
                rules+="-A FORWARD -p tcp -o {} --sport {}:{} --dport {} -j {}\n".format(itf, r[0],r[1],dport,action)
                #rules+="-A FORWARD -p udp -o {} --sport {}:{} --dport {} -j {}\n".format(itf, r[0],r[1],dport,action)
                

        rules+='COMMIT'
            
        
        return rules
        
    def get_random_inv(self):
        dport=random.choice(list(self.port_acl_stat.keys()))
        rngs=self.get_ranges(dport)
        entry=random.choice(rngs)
        sport=random.randrange(entry[0], entry[1]+1)
        return sport, dport, entry[2]
        

class Row:
    def __init__(self, linenum, row):
        self.linenum=linenum
        self.sport = int(row[0])
        self.dport = int(row[1])
        self.action = row[4]

    def dump(self):
        print("line {} : {} {} {}".format(self.linenum, self.sport, self.dport, self.action))


#fp=FirewallParser("firewall_log/log2.csv",42)
#print(fp.generate_rules('eth1'))
