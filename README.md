# DOQP

## Compile
<pre><code>make clean sgx</code></pre>

## Run
<pre><code>
./app select tables DATA_FILE QUERY_FILE HTree/DO constant
./app join tables DATA_FILE1 DATA_FILE2 HTree/DO/CZSC21 constant pf-join/general
./app select-join tables DATA_FILE1 DATA_FILE2 HTree constant general QUERY_FILE
</code></pre>

## Headers 
Include/CONFIG.hpp, choose the corresponding testSizeVec definition

