#!/usr/bin/python
'''Install UPC++ from the bitbucket repo, which includes checking out
a fresh code of the code, bootstrap, configure, make, and install.

Copyright: https://bitbucket.org/upcxx/upcxx/wiki/License
'''

import sys
import os
import datetime
import time
import re
from optparse import OptionParser
from subprocess import call
from subprocess import check_call
from subprocess import check_output
import ntpath

VERBOSE=True

upcxx_repo = "http://bitbucket.org/upcxx/upcxx"
array_repo = "http://bitbucket.org/upcxx/upcxx-arrays"

array_config_option = " --enable-md-array "
shortname_config_option = " --enable-short-names "

global_run_mode = 2

### configure commands for known systems, need to substitue the actual
### gasnet path and installation path before use.
predefined_configs = \
    {"crayxe"  : ["cross-configure-crayxe-linux", "gemini"],
     "crayxc"  : ["configure CC=cc CXX=CC", "aries"],
     "macmpi" : ["configure CC=clang CXX=mpicxx", "mpi"],
     "mac" : ["configure CC=clang CXX=clang++", "smp"],
     "linuxmpi":["configure CXX=mpicxx", "mpi"],
     "linux":["configure", "smp"]}

def print_info(info):
    print("***** Info: %s *****" % info)

## run_mode: 0 - print only, 1 - print and run , 2 - run only
def run_shell(shell_command):
    run_mode = global_run_mode
    if (run_mode < 2):
        print('Execute shell command "%s"' % shell_command)

    if (run_mode == 0):
        return

    check_call(shell_command, shell=True)

    print('shell command "%s" completes successfully.' % shell_command)

def get_full_configure(configure, gasnet_path, gasnet_conduit, thread_mode, installation_path, additional_options):
    full_configure = "%s --with-gasnet=%s/include/%s-conduit/%s-%s.mak --prefix=%s %s" % (configure, gasnet_path, gasnet_conduit, gasnet_conduit, thread_mode, installation_path, additional_options)
    ## print("full_configure %s" % full_configure)
    return full_configure

def prelude():
    '''Commands for setting up the shell environment before doing the real work
    '''
    commands = ["module load git", "module load gcc"]
    for c in commands:
        print("Executing: %s" % c)
        #check_call(c)

def path_leaf(path):
    head, tail = ntpath.split(path)
    return tail or ntpath.basename(head)

def checkout(repo_url, branch, dest=""):
    clone_repo = "git clone -b %s %s %s" % (branch, repo_url, dest)
    print("clone repo command: %s" % clone_repo)
    run_shell(clone_repo)

#def build_gasnet(gasnet_url):
    ## get the gasnet tar ball


def main():
    pwd = os.getcwd()
    time_stamp = time.strftime("%Y%m%d-%H%M%S")
    #today = datetime.date.today().isoformat()

    ## Trying to find out where is gasnet from the the default path
    try:
        upcc_path = check_output("which upcc", shell=True)
        gasnet_path = os.path.dirname(os.path.dirname(upcc_path)) + "/opt"
        print("gasnet_path: %s" % gasnet_path)
    except:
        print("Cannot find Berkeley UPC (upcc) in the default path")
        upcc_path = ""
        gasne_path = ""

    ## Parse command options

    parser = OptionParser()

    parser.add_option("-b", "--upcxx-branch", action="store", type="string", dest="upcxx_branch", help="Git branch of upcxx to be checked out", default="develop")

    parser.add_option("-p", "--prefix", action="store", type="string", dest="install_prefix", help="Installation path prefix", default=pwd+"/upcxx-install-"+time_stamp)

    parser.add_option("--build-dir", action="store", type="string", dest="build_dir", help="Directory path for building UPC++", default=pwd+"/upcxx-build-"+time_stamp)

    parser.add_option("--md-arrays", action="store_true", dest="install_arrays", help="Install the multidimensional arrays feature", default=False)

    parser.add_option("--array-branch", action="store", type="string", dest="array_branch", help="Git branch of UPC++ arrays to be checked out", default="develop")

    parser.add_option("--gasnet", action="store", type="string", dest="gasnet_path", help="GASNet Path", default=gasnet_path)

    parser.add_option("--gasnet-conduit", action="store", type="string", dest="gasnet_conduit", help="GASNet conduit name", default="")

    parser.add_option("--thread-mode", action="store", type="string", dest="thread_mode", help="GASNet thread mode: seq, par, parsync", default="seq")

    parser.add_option("--gasnet-url", action="store", type="string", dest="gasnet_url", help="URL for downloading the GASNet tar ball", default="gasnet.lbl.gov")

    parser.add_option("--config", action="store", type="string", dest="upcxx_config", help="Predefined configure options for a known system", default="")

    parser.add_option("-n", "--dry-run", action="store_true", dest="is_dry_run", help="Just print out the commands without executing them if this option is enabled", default=False)

    (options, args) = parser.parse_args()

    if (options.is_dry_run):
        global global_run_mode;
        global_run_mode = 0

    gasnet_conduit = options.gasnet_conduit
    if (options.upcxx_config != ""):
        try:
            configure = predefined_configs[options.upcxx_config][0]
            if (gasnet_conduit == ""):
                gasnet_conduit = predefined_configs[options.upcxx_config][1]
        except NameError:
            print("%s is not a valid predefined config!" % options.upcxx_config)
            sys.exit(1)
    else:
        configure = "configure"

    if (gasnet_conduit == ""):
        gasnet_conduit = "smp"

    if (VERBOSE):
        print("upcxx_branch: %s" % options.upcxx_branch)
        print("install_prefix: %s" % options.install_prefix)
        print("build_dir: %s" % options.build_dir)
        print("install md-arrays?: %s" % options.install_arrays)
        print("array_branch: %s" % options.array_branch)
        print("gasnet path: %s" % options.gasnet_path)
        print("gasnet conduit: %s" % gasnet_conduit)
        print("multithread mode: %s" % options.thread_mode)
        print("gasnet_url: %s" % options.gasnet_url)
        print("upcxx_config: %s" % options.upcxx_config)

    prelude()

    ## Checkout the source code from the git repositories

    build_dir = options.build_dir + "/build"
    src_dir = options.build_dir + "/upcxx_src"

    if (global_run_mode):
        ##os.mkdir(options.build_dir)
        ##os.chdir(options.build_dir)
        checkout(upcxx_repo, options.upcxx_branch, dest=src_dir)

        if options.install_arrays:
            upcxx_array_path = src_dir + "/include/upcxx-arrays"
            print ("upcxx_array_path: %s" % upcxx_array_path)
            checkout(array_repo, options.array_branch, dest=upcxx_array_path)

        os.chdir(src_dir)
        run_shell("./Bootstrap.sh") ##

    configure = src_dir + "/" + configure

    ## Bootstrap os.chdir(src_dir) call("./Bootstrap.sh")

    additional_options = shortname_config_option
    if (options.install_arrays):
        additional_options += array_config_option

    ## Prepare the configure command
    configure_command = get_full_configure(configure,
                                           options.gasnet_path,
                                           gasnet_conduit,
                                           options.thread_mode,
                                           options.install_prefix,
                                           additional_options)
    print("configure_command: %s" % configure_command)

    if (global_run_mode):
        run_shell("mkdir -p %s" % build_dir)
        os.chdir(build_dir)

    run_shell(configure_command)

    ## The hard part is done not it's the easy one

    print_info("Finished the configure step, start to build (make) in %s" % os.getcwd())
    run_shell("make");
    run_shell("make install");

    ## All done print("UPC++ installation successfully completed!")

if __name__ == '__main__': main()
