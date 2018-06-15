# bypassuac


###example:

```
bypassuac.exe "/c echo 1 >> c:\\3333"
bypassuac.exe "/c powershell -c "aaaa"
bypassuac,exe system
```


![](https://raw.githubusercontent.com/liuxigu/bypassuac/master/uac.png)

***

![](https://raw.githubusercontent.com/liuxigu/bypassuac/master/getsystem.png)


***

###题外话:

只能编译成32位. 多重指针写起来太麻烦,所以patch peb用内嵌汇编写的. 想编译成x64的话得改改代码.    
参考了老外的项目：https://github.com/FuzzySecurity/PowerShell-Suite/tree/master/Bypass-UAC          
patch peb后,然后调用IFileOperation对象的某个接口的shellexecute函数,原因在老外的文章里写了.      
具体细节看代码吧.     
适用于Win7-Win10     

***

加了一段getsystem的代码,原理跟msf里的getsystem一样.