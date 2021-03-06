@*
MIT License

Copyright (c) 2019 Michel Dénommée

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*@
@ns     
@name   user_info @** comment ....

@** include common.h relative to source view directory
@include @* comment *@ "@dirname/../../common.h" @** line comment

@header{
    
    class abc
    {
    public:
        char a;
        char b;
        char c;
    };
    
}<br/>

@(md){
# header AAA
_italic text_

``` sh
$ echo("some bash code!!!")
```

~~~ c++
// this is c++ code
int i = j;
class test
{
    test()
    {
    }
};
~~~

# header BBB
*_bold italic text_* 
@} this must be inside markdown block

* item a
* item b

}

@{
    @this->write_enc("<div></div>");
    int i = 1;
    @(htm){<div id="@:"\"a\"";">@(md){_italic_ *bold*}</div>}
}<br/>

<div>
    username: @get("username", "abc")<br/>
    fullname: @get("fullname", "def 123")<br/>
</div>

@header{
}

<br/>