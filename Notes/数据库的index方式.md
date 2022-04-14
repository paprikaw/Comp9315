# Signature:

注意：Signature只适合equality test

我们希望使用一个signature来表示一个tuple。这样可以给我们一个体积的signature 文件，更加适合扫描。

在signature中，我们将每个attribute的信息都映射到里面。这样就可以支持单attribute的扫描

* 构建sinature的方式(SMIC)
* 使用signatre进行tuple scanning
  * 在什么情况下能够match？
  * 使用page by page的方式，使用signature找找到所有可能包含match tuple的page，然后在最后阶段将page加载到buffer中进行检查。(Page的加载时间远远超过内存计算)
* signature 的false match
* 为每一个tuple存一个signature可能不够高效，是否可以存一个page signatrue 告诉我们一个page里面是否有可能存在tuple
