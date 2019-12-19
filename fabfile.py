import os
import getpass
import datetime
from fabric.api import *

lightkone_home="./Yggdrasil-LowLevelLib"
lightkone_protos_home="./Yggdrasil"

lightkone_remote="~/Yggdrasil-LowLevelLib"
lightkone_protos_remote="~/Yggdrasil"

#raspis = ['192.168.1.101']

raspis = [
    '192.168.1.101','192.168.1.102','192.168.1.103','192.168.1.104','192.168.1.105',
    '192.168.1.106','192.168.1.107','192.168.1.108','192.168.1.109','192.168.1.110',
    '192.168.1.111','192.168.1.112','192.168.1.113','192.168.1.114','192.168.1.115',
    '192.168.1.116','192.168.1.117','192.168.1.118','192.168.1.119','192.168.1.120',
    '192.168.1.121'
    ]


#raspis = [
#    '192.168.1.101','192.168.1.102','192.168.1.103','192.168.1.104','192.168.1.105',
#    '192.168.1.106','192.168.1.107','192.168.1.108','192.168.1.109','192.168.1.110',
#    '192.168.1.111','192.168.1.112','192.168.1.113','192.168.1.114','192.168.1.115',
#    '192.168.1.116','192.168.1.117','192.168.1.118','192.168.1.119','192.168.1.120',
#    '192.168.1.121','192.168.1.122','192.168.1.123','192.168.1.124'
#    ]


#UP
#raspis = ['192.168.1.101','192.168.1.102','192.168.1.106','192.168.1.107','192.168.1.114','192.168.1.122']

#DOWN
#raspis = ['192.168.1.120','192.168.1.121','192.168.1.122','192.168.1.123']

def __get_hosts(hosts_dict, hosts_per_dc):
    hosts = []
    hosts = raspis
    return hosts


env.user = getpass.getuser()
env.disable_known_hosts = True
env.userKey = '~/.ssh/id_rsa'

@parallel
@hosts(raspis)
def setethers():
    run('sudo cp ~/tools/ethers /etc/')

@parallel
@hosts(raspis)
def setarptable():
    run('sudo arp -f')

@parallel
@hosts(raspis)
def clock():
    run('date');

@parallel
@hosts(raspis)
def synclock():
#    env.date = local('date')
    now = datetime.datetime.now()
    local('ssh -o StrictHostKeyChecking=no -i ' + env.userKey + ' ' + env.user + '@' + env.host + ' "sudo date +\'%Y-%m-%d %T.%N\' -s \'' + str(now) + '\'"')


@parallel
@hosts(raspis)
def sync():
    local('rsync -avz --delete --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + lightkone_home + ' ' + env.user + '@%s:~/' % (env.host))
    local('rsync -avz --delete --exclude "*CMakeFiles*" --exclude "*.o" --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' ' + lightkone_protos_home + ' ' + env.user + '@%s:~/' % (env.host))
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' CMakeLists.txt ' + env.user + '@%s:~/' % (env.host))
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' tools ' + env.user + '@%s:~/' % (env.host))

@parallel
@hosts(raspis)
def createLighkoneConfigDir():
    run("sudo mkdir /etc/lightkone | 0; sudo mkdir /etc/lightkone/topologyControl | 0; sudo chmod -R 777 /etc/lightkone")

@parallel
@hosts(raspis)
def createYggdrasilHome():
    run("sudo mkdir /home/yggdrasil; sudo chmod -R 755 /home/yygdrasil")

@parallel
@hosts(raspis)
def configureYggdrasilHome():
    run("sudo mkdir /home/yggdrasil/bcast; sudo mkdir /home/yggdrasil/cmd; sudo mkdir /home/yggdrasil/bin")


@parallel
@hosts(raspis)
def updateYggdrasilControl():
    run("sudo cp ~/bin/YggdrasilControlProcess /home/yggdrasil/; sudo chmod 777 /home/yggdrasil/YggdrasilControlProcess")

@parallel
@hosts(raspis)
def addBinToYggdrasilHome():
    run("sudo cp /home/"+ (env.user) +"/bin/cmd* /home/yggdrasil/cmd; sudo cp /home/"+ (env.user) +"/bin/bcast_generic_test /home/yggdrasil/bin")
    run("sudo chmod 777 /home/yggdrasil/cmd/*; sudo chmod 777 /home/yggdrasil/bin/*")

@parallel
@hosts(raspis)
def cleanYggdrasilHome():
    run("sudo rm -rf /home/yggdrasil/*")

@hosts(raspis)
def checkInforumLogs():
    run("ls -aloh /home/yggdrasil/inforum18")

@hosts(raspis)
def getInforumLogs():
    get("/home/yggdrasil/inforum18/*")

@hosts(raspis)
def getBcastLogs():
    get("/home/yggdrasil/bcast/*")

@hosts(raspis)
def backupInforumLogs():
    run("sudo mv /home/yggdrasil/inforum18 /home/yggdrasil/inforum18-remote1")
    run("sudo mkdir /home/yggdrasil/inforum18")

@parallel
@hosts(raspis)
def updateTopology():
    local('rsync -avz --rsh="ssh -o StrictHostKeyChecking=no" -i ' + env.userKey + ' topologyControl/macAddrDB.txt ' + env.user + '@%s:/etc/lightkone/topologyControl/' % (env.host))
    local('scp -i ' + env.userKey + ' topologyControl/'+(env.host)+ ' ' + env.user + '@%s:/etc/lightkone/topologyControl/neighs.txt' % (env.host))
    run("sudo chmod -R 777 /etc/lightkone")

@parallel
@hosts(raspis)
def removeTopology():
    run("sudo rm /etc/lightkone/topologyControl/neighs.txt")

@parallel
@hosts(raspis)
def test():
    run("hostname")


@parallel
@hosts(raspis)
def reboot():
    run("sudo reboot")


@parallel
@hosts(raspis)
def shutdown():
    run("sudo shutdown now")

@parallel
@hosts(raspis)
def configureboot():
    with cd("~/tools/"):
        run("sudo mv lightkone.sh /etc/init.d/")
        run("sudo chmod +x /etc/init.d/lightkone.sh")
        run("sudo cp *.sh /home/yggdrasil/")
	run("sudo chmod +x /home/yggdrasil/*.sh")
        run("sudo cp runonboot.txt /home/yggdrasil/")
    with cd("/etc/init.d"):
        run("sudo update-rc.d lightkone.sh defaults")

@parallel
@hosts(raspis)
def build():
    run("cmake .; make")

@parallel
@hosts(raspis)
def rebuild():
    run("make")

@parallel
@hosts(raspis)
def runtest(test):
    with cd("~/tools/"):
        run("/usr/bin/nohup ./run.sh " + test)

@parallel
@hosts(raspis)
def runtestwitharg(test, arg):
    with cd("~/tools/"):
        run("/usr/bin/nohup ./runWithArg.sh " + test + " " + arg)

@parallel
@hosts(raspis)
def checktestlive(test):
    run("sudo ps ax | grep '"+test+"' | wc -l")

@parallel
@hosts(raspis)
def stopcontrol():
    run("sudo killall -3 YggdrasilControlProcess")

@parallel
@hosts(raspis)
def stoptest(test):
    run("sudo killall -3 " + test)

@parallel
@hosts(raspis)
def killtest(test):
    run("sudo killall -9 " + test)

@parallel
@hosts(raspis)
def armageddon():
    run("sudo rm -rf ~/*")

@parallel
@hosts(raspis)
def rmnohup():
    run("sudo rm tools/nohup.out")

@parallel
@hosts(raspis)
def getBcastResults():
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.111:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.112:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.123:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.120:/home/andreffrosa/results/ .")
    run("sudo scp -o StrictHostKeyChecking=no -i " + env.userKey + " -r 192.168.137.119:/home/andreffrosa/results/ .")


@parallel
@hosts(raspis)
def runBcastTests1():
    run("sudo ./bin/bcast_test -r -2 -t 0 -c 0 -i 5000 -s 1000 -b -p 0.5 -d 60 >> ./results/flooding+nullTimer.txt")


@parallel
@hosts(raspis)
def cleanHome():
    run("sudo rm -rf /home/andreffrosa/*")

@parallel
@hosts(raspis)
def createBcastFolder():
    run("sudo mkdir /home/yggdrasil/bcast_results/")

