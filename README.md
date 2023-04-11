# DOQP

## Compile
<pre><code>make clean sgx</code></pre>

## Run
<pre><code>
./app select tables DATA_FILE QUERY_FILE HTree/DO constant
./app join tables DATA_FILE1 DATA_FILE2 HTree/DO/CZSC21 constant pf-join/general
./app select-join tables DATA_FILE1 DATA_FILE2 HTree constant general QUERY_FILE
</code></pre>

<br>HTree: Hierarchical dp histogram [link][3]
<br>DO: PrivTree dp histogram [link][1]
<br>CZSC21: Implementation of [link][2]

## Headers 
Include/CONFIG.hpp, choose the corresponding testSizeVec definition

[1]: <https://dl.acm.org/doi/pdf/10.1145/2882903.2882928>
[2]: <https://eprint.iacr.org/2021/593.pdf>
[3]: <https://dl.acm.org/doi/pdf/10.14778/2556549.2556576>
