Introducing Bitcoin Music
-------------------------

Bitcoin Music is a blockchain which uses Delegated Proof Of Stake (DPOS) as a consensus algorithm.

  - Currency Symbol BTCM
  - Approximately 0.25% APR long term inflation rate


No Support & No Warranty 
------------------------
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Build Status
------------

**master** | **develop** | **next_hardfork**
 --- | --- | ---
 [![master Build Status](https://travis-ci.org/btcm/btcm-Source.svg?branch=master)](https://travis-ci.org/btcm/btcm-Source) | [![develop Build Status](https://travis-ci.org/btcm/btcm-Source.svg?branch=develop)](https://travis-ci.org/btcm/btcm-Source) | [![next_hardfork Build Status](https://travis-ci.org/btcm/btcm-Source.svg?branch=next_hardfork)](https://travis-ci.org/btcm/btcm-Source) | [![testnet Build Status](https://travis-ci.org/btcm/btcm-Source.svg?branch=testnet)](https://travis-ci.org/btcm/btcm-Source)

Code is Documentation
---------------------

Rather than attempt to describe the rules of the blockchain, it is up to
each individual to inspect the code to understand the consensus rules.  

Build Instructions
------------------
These instructions apply on Fedora 24. For other distributions, make sure you have 
installed Boost 1.60-1.66 first. You also have instructions to build test net here.
For building, you need at least 4 GB of memory available.

We are testing using Fedora 25, x64

    sudo dnf clean metadata
    sudo dnf install automake autoconf libtool make cmake gcc flex bison doxygen gettext-devel git readline-devel openssl-devel libcurl-devel ncurses-devel boost-devel boost-static gcc-c++
    mkdir -p ~/dev/btcm
    cd ~/dev/btcm
    git clone https://github.com/btcm/btcm-Source.git
    cd ~/dev/btcm/btcm-Source
    git submodule update --init --recursive
    mkdir -p ~/dev/btcm/btcm-build
    cd ../btcm-build
    #optionally specify Boost root directory in case it is not installed on standard paths with -DBOOST_ROOT=/location/
    
    #For production build
    cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release ~/dev/btcm/btcm-Source/
    #For test net build
    cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug ~/dev/btcm/btcm-Source/
    
    #optionally, install the binaries
    cmake --build . --target all -- -j 3
    
Now the binaries shall be in directory ~/dev/btcm/btcm-build/programs/

Seed Nodes
----------
    btcm.seeds.quisquis.de:1027 (production)
    btcm-test.seeds.quisquis.de:33327 (for testnet)

How to Start
------------
First, start btcm daemon:
(Witness will need additional config)

    cd ~/dev/btcm/btcm-build
    #For production: (genesis is embedded)
    ./programs/btcmd/btcmd --rpc-endpoint=0.0.0.0:1028
   
    #For testnet
    ./programs/btcmd/btcmd --rpc-endpoint=0.0.0.0:33328
    
The daemon is ready. The JSON-RPC is listening on localhost on port 1028 (or 33328, respectively).

You may want to start it using `screen` to make server available after terminal is closed.
Install screen with `dnf install screen`
Start a new screen with command `screen`
Then run `./btcmd` with the right parameters
You can reconnect to the screen later using `screen -r`

For witness setup
------------
Here are the steps to setup a witness for the testnet
Ensure you have opened ports in your firewall.

    1- Go to http://FIXME/ws/accounts to generate your keys.
    Use generate keys, write stuff, as long as it is random), just to generate a pair.
    2- Note private and public owner keys generated
    3- Prepare your test net build using instruction above
    4- Launch node again:
    cd %btcm build folder%/btcmd/
    ./btcmd
    5- Edit config: from current dir, witness_node_data_dir/config.ini
    6- Set value of witness your account, and private-key to the private owner wif generated, then save file
    You may also want to set enable-stale-production to true
    7- Launch node again:
    ./btcmd --rpc-endpoint=127.0.0.1:1028
    8- btcm is running... in another session, go to %btcm build folder%/cli_wallet
    9- Run cli_wallet:
    ./cli_wallet
    10- set_password $your_wallet_pass...%
    11- unlock "pass..."
    12- import_key %owner priv key%
    Use http://FIXME/ws/accounts to reobtain keys using your password if you don't have them
    13- import key %active priv key%
    Use http://FIXME/ws/accounts to reobtain keys using your password if you don't have them
    14- use list_my_accounts, see if you see your account
    15- get some vesting to transact...
    16- announce yourself as a witness, change placeholders: update_witness "WitnessAccountName" "http://%???%" %witness_pub_key% {} true
    17- get votes... use wallet to vote, ask someone to vote
    18- at this point, you have votes, your node may be selected as an active witness and start produce blocks
