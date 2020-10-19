var xmlhttp;
var currentJSONModel;
var nodeIndex = 4;
var d;
var fileListInfo;
var currentDisplayNode = null;

function loadCurrentModle()
{
    //load list
    xmlhttplist=new XMLHttpRequest();
    xmlhttplist.onreadystatechange = displayModleList_Callback;
    xmlhttplist.open("GET","dealModel.php?cmd=modellist",true);
    xmlhttplist.send();
}
function displayModle_Callback()
{
    if (xmlhttp.readyState!=4 || xmlhttp.status!=200) {
        return ;
    }
    currentJSONModel = JSON.parse(xmlhttp.responseText);
    displayModle();
}
function displayModle()
{
    var CurrentConfigFileHand = document.getElementById("CurrentConfigFileHand");
    // CurrentConfigFileHand.innerHTML = "<div class=\"nodeAttrTitle\" style=\"font-family:auto;\">&nbsp;文件信息</div>";
    // CurrentConfigFileHand.appendChild(document.createTextNode(`名称：${currentJSONModel.Name}`));
    // CurrentConfigFileHand.appendChild(document.createElement('br'));
    // CurrentConfigFileHand.appendChild(document.createTextNode(`作者：${currentJSONModel.Author}`));
    // CurrentConfigFileHand.appendChild(document.createElement('br'));
    // CurrentConfigFileHand.appendChild(document.createTextNode(`日期：${currentJSONModel.Date}`));
    // CurrentConfigFileHand.appendChild(document.createElement('br'));
    // CurrentConfigFileHand.appendChild(document.createTextNode(`摘要：${currentJSONModel.Summary}`));
    document.getElementById("configFileName").value = currentJSONModel.Name;
    document.getElementById("configFileAuthor").value = currentJSONModel.Author;
    document.getElementById("configFileDate").value = currentJSONModel.Date;
    document.getElementById("configFileSummary").value = currentJSONModel.Summary;
    d = new dTree('d');
    d.add(0,-1,'OPC UA NameSpace');
    d.add(1,0,'Objects',`javascript:displayNameSpace()`);
    d.add(2,0,'Types','','','',"img/folderopen.gif");
    d.add(3,0,'Views','','','',"img/folderopen.gif");
    nodeIndex = 4;
    for (var i=0;i<currentJSONModel.AddressSpace.length;i++)
    {
        creatModelTree(currentJSONModel.AddressSpace[i],1);
    }
    document.getElementById("devTree").innerHTML = '<div class="nodeAttrTitle">&nbsp;地址空间</div>';
    document.getElementById("devTree").innerHTML += d;
    d.openAll();
    // displayAttr(currentJSONModel.AddressSpace[0].NodeID_ID);
    displayNameSpace();
}
function displayNameSpace(){
    if (currentDisplayNode!=null) {
        saveAttrToVar(currentDisplayNode);
    }
    currentDisplayNode = null;
    document.getElementById("baseAttributes").style.display="none";
    document.getElementById("variableAttributes").style.display="none";
    document.getElementById("bottomProtocol").style.display="none";
    document.getElementById("childrenNodeList").innerHTML = `
    <tr>
        <th style="width: 30%;"><span>子节点名</span></th>
        <th style="width: 30%;">节点类型</th>
        <th>操作</th>
    </tr>
    `;
    if (currentJSONModel.AddressSpace != undefined) {
        for (tempChildIndex in currentJSONModel.AddressSpace){
            var tempItem = `
            <tr>
                <td>
                    <input disabled="value" type="text" name="ChildName" value="${currentJSONModel.AddressSpace[tempChildIndex].BrowseName}">
                </td>
                <td>
                    <select disabled="value" name="ChildNodeClass" style="width: 90%;">
                        <option value="${currentJSONModel.AddressSpace[tempChildIndex].NodeClass}">${currentJSONModel.AddressSpace[tempChildIndex].NodeClass}</option>
                    </select></td>
                <td>
                    <button type="button" onclick="javascript:deleteNode('${currentJSONModel.AddressSpace[tempChildIndex].NodeID_ID}')">删除节点</button>
                </td>
            </tr>
            `
            document.getElementById("childrenNodeList").innerHTML += tempItem;
            // console.log(currentJSONModel.AddressSpace[tempChildIndex].BrowseName);
        }
    }
    document.getElementById("childrenNodeList").innerHTML += `                                <tr>
    <tr>
        <td>
            <input type="text" name="ChildName" value="新节点" id="newNodeName">
        </td>
        <td>
            <select name="ChildNodeClass" style="width: 90%;" id="newNodeClass">
            <option value="Object">Object</option>
            <option value="Variable">Variable</option>
            </select></td>
        <td>
            <button type="button" onclick="javascript:addNodeToASP()">添加节点</button>
        </td>
    </tr>
    `;
}
function creatModelTree(UA_Node,parentsIndex)
{
    if (UA_Node.NodeClass == "Object")
        d.add(nodeIndex,parentsIndex,UA_Node.BrowseName,`javascript:displayAttr(\'${UA_Node.NodeID_ID}\')`,'','',"img/obj.png","img/obj.png");
    if (UA_Node.NodeClass == "Variable")
        d.add(nodeIndex,parentsIndex,UA_Node.BrowseName,`javascript:displayAttr(\'${UA_Node.NodeID_ID}\')`,'','',"img/tip.png","img/tip.png");
    var myself = nodeIndex++;
    if ('ChildNode' in UA_Node){
        for (var i=0;i<UA_Node.ChildNode.length;i++)
        {
            creatModelTree(UA_Node.ChildNode[i],myself);
        }
    }
}
function getUANodeById(name)
{
    function findInChildren(node)
    {
        if (node.NodeID_ID==name)
            return node;
        if ('ChildNode' in node) 
            for (childNode in  node.ChildNode){
                var temp = findInChildren(node.ChildNode[childNode],name);
                if (temp != null) return temp;
            }
        return null;
    }
    for (node in currentJSONModel.AddressSpace){
        if (currentJSONModel.AddressSpace[node].NodeID_ID==name) 
            return currentJSONModel.AddressSpace[node];
        if ('ChildNode' in currentJSONModel.AddressSpace[node]){
            for (childNode in  currentJSONModel.AddressSpace[node].ChildNode){
                var temp = findInChildren(currentJSONModel.AddressSpace[node].ChildNode[childNode]);
                if (temp != null) return temp;
            }
        }
    }
    return null;
}
function displayAttr(nodeid)
{
    document.getElementById("baseAttributes").style.display="block";
    if (currentDisplayNode!=null) {
        saveAttrToVar(currentDisplayNode);
    }
    currentDisplayNode = nodeid;
    var attr = getUANodeById(nodeid);//根据id名称获取节点对象
    var tempOptions ;
    //nodeID地址空间选择
    document.getElementById("NodeID_NamespaceIndex").options[parseInt(attr.NodeID_NamespaceIndex)].selected = true;
    tempOptions = document.getElementById("NodeID_Type").options;//nodeid类型选择
    for (tempOption in tempOptions){
        if (tempOptions[tempOption].value == attr.NodeID_IDType){
            tempOptions[tempOption].selected = true;
            break;
        }
    }
    document.getElementById("NodeID").value = attr.NodeID_ID;//nodeid
    tempOptions = document.getElementById("NodeClass").options;//节点类型选择
    for (tempOption in tempOptions){
        if (tempOptions[tempOption].value == attr.NodeClass){
            tempOptions[tempOption].selected = true;
            break;
        }
    }
    document.getElementById("DisplayName").value = attr.DisplayName;//DisplayName
    document.getElementById("BrowseName").value = attr.BrowseName;//BrowseName
    document.getElementById("QualifiedName").value = attr.QualifiedName;//QualifiedName
    document.getElementById("Description").value = attr.Description;//Description
    tempOptions = document.getElementById("ReferenceTypeId").options;//与父节点的引用关系
    for (tempOption in tempOptions){
        if (tempOptions[tempOption].value == attr.ReferenceTypeId){
            tempOptions[tempOption].selected = true;
            break;
        }
    }
    /************孩子节点 占位*************/
    document.getElementById("childrenNodeList").innerHTML = `                                <tr>
    <th style="width: 30%;"><span>子节点名</span></th>
        <th style="width: 30%;">节点类型</th>
        <th>操作</th>
    </tr>
    `;
    if (attr.ChildNode != undefined) {
        for (tempChildIndex in attr.ChildNode){
            var tempItem = `
            <tr>
                <td>
                    <input disabled="value" type="text" name="ChildName" value="${attr.ChildNode[tempChildIndex].BrowseName}">
                </td>
                <td>
                    <select disabled="value" name="ChildNodeClass" style="width: 90%;">
                        <option value="${attr.ChildNode[tempChildIndex].NodeClass}">${attr.ChildNode[tempChildIndex].NodeClass}</option>
                    </select></td>
                <td>
                    <button type="button" onclick="javascript:deleteNode('${attr.ChildNode[tempChildIndex].NodeID_ID}')">删除节点</button>
                </td>
            </tr>
            `
            document.getElementById("childrenNodeList").innerHTML += tempItem;
            // console.log(attr.ChildNode[tempChildIndex].BrowseName);
        }
    }
    document.getElementById("childrenNodeList").innerHTML += `                                <tr>
    <tr>
        <td>
            <input type="text" name="ChildName" value="新节点" id="newNodeName">
        </td>
        <td>
            <select name="ChildNodeClass" style="width: 90%;" id="newNodeClass">
            <option value="Object">Object</option>
            <option value="Variable">Variable</option>
            </select></td>
        <td>
            <button type="button" onclick="javascript:addChildNode()">添加节点</button>
        </td>
    </tr>
    `;
    /*************变量类型***************/
    if (attr.NodeClass=='Variable'){//判断节点类型，
        document.getElementById("variableAttributes").style.display="block";
        document.getElementById("bottomProtocol").style.display="block";
    }else{
        document.getElementById("variableAttributes").style.display="none";
        document.getElementById("bottomProtocol").style.display="none";
        return;
    }
    tempOptions = document.getElementById("DataType").options;//容器数据类型
    for (tempOption in tempOptions){
        if (tempOptions[tempOption].value == attr.DataType){
            tempOptions[tempOption].selected = true;
            break;
        }
    }
    document.getElementById("InitialValue").value = attr.InitialValue;//Description
    tempOptions = document.getElementById("BottomProtocolType").options;//容器数据类型
    for (tempOption in tempOptions){
        if (tempOptions[tempOption].value == attr.BottomProtocolType){
            tempOptions[tempOption].selected = true;
            break;
        }
    }
    document.getElementById("MB_MachineAddress").value = attr.MB_MachineAddress;//机器地址
    document.getElementById("MB_FunctionCode").value = attr.MB_FunctionCode;//机器地址
    document.getElementById("MB_DataBitSize").value = attr.MB_DataBitSize;//机器地址
    document.getElementById("MB_RegisterAddress").value = attr.MB_RegisterAddress;//机器地址
    document.getElementById("RefreshFrequency").value = attr.RefreshFrequency;//机器地址
    document.getElementById("MappingCoefficient").value = attr.MappingCoefficient;//机器地址
    document.getElementById("InitialValue").value = attr.InitialValue;//Description
    tempOptions = document.getElementById("BottomDataType").options;//容器数据类型
    for (tempOption in tempOptions){
        if (tempOptions[tempOption].value == attr.BottomDataType){
            tempOptions[tempOption].selected = true;
            break;
        }
    }
}

function displayModleList_Callback()
{
    if (xmlhttplist.readyState!=4 || xmlhttplist.status!=200) {
        return ;
    }
    if (xmlhttplist.responseText.slice(0,3)=='Fail') {
        alert("服务器出错：无法连接数据库！");
        return;
    }
    fileListInfo = JSON.parse(xmlhttplist.responseText);
    document.getElementById("CurrentConfigFileName").innerHTML = "";
    for (temp in fileListInfo)
    {
        var tempDiv = document.createElement('div');
        // tempDiv.appendChild(document.createTextNode(fileListInfo[temp].fileName));
        tempDiv.className="configFileNameTag";
        var tempImg = document.createElement("img");
        tempImg.src = "./img/doc.png";
        tempDiv.appendChild(tempImg);
        // tempDiv.innerHTML = `<img src="./img/doc.png" >`;
        tempDiv.appendChild(document.createTextNode("   " + fileListInfo[temp].fileName));
        if (fileListInfo[temp].isUsed=='1'){
            tempDiv.style.border = " 1px #5b6980 solid";
            tempDiv.style.backgroundColor = "#aab3c1";
            tempDiv.innerHTML += "<span style='color:#5b6980;'>  [已应用]</span>";
            xmlhttp=new XMLHttpRequest();
            xmlhttp.onreadystatechange = displayModle_Callback;
            xmlhttp.open("GET",`dealModel.php?cmd=requestmodel&name=${fileListInfo[temp].fileName}`,true);
            xmlhttp.send();
        }
        // tempDiv.addEventListener("click",selectConfigFile,true);
        document.getElementById("CurrentConfigFileName").appendChild(tempDiv);
    }
    var tempdivlist = document.getElementById("CurrentConfigFileName").getElementsByTagName("div");
    for (var i=0;i<tempdivlist.length;i++)
    {
        tempdivlist[i].addEventListener("click",selectConfigFile);
    }
}
function selectConfigFile(fileID)
{
    // console.log(fileID);
    currentDisplayNode = null;
    if (fileID.toElement.tagName!="DIV") return ;
    xmlhttp=new XMLHttpRequest();
    xmlhttp.onreadystatechange = displayModle_Callback;
    xmlhttp.open("GET",`dealModel.php?cmd=requestmodel&name=${fileID.toElement.innerText.split(' ')[1]}`,true);
    xmlhttp.send();
    var tempdivlist = document.getElementById("CurrentConfigFileName").getElementsByTagName("div");
    for (var i=0;i<tempdivlist.length;i++)
    {
        tempdivlist[i].style.border = " 0px #5b6980 solid";
        tempdivlist[i].style.backgroundColor = "#e3e9ee";
    }
    fileID.toElement.style.border = " 1px #5b6980 solid";
    fileID.toElement.style.backgroundColor = "#aab3c1";
}
function saveAttrToVar(nodeid)
{
    var thisNode = getUANodeById(nodeid);
    thisNode.NodeID_NamespaceIndex = document.getElementById("NodeID_NamespaceIndex").value;
    thisNode.NodeID_IDType = document.getElementById("NodeID_Type").value;
    thisNode.NodeID_ID = document.getElementById("NodeID").value;
    thisNode.NodeClass = document.getElementById("NodeClass").value;
    thisNode.DisplayName = document.getElementById("DisplayName").value;
    thisNode.BrowseName = document.getElementById("BrowseName").value;
    thisNode.QualifiedName = document.getElementById("QualifiedName").value;
    thisNode.Description = document.getElementById("Description").value;
    thisNode.ReferenceTypeId = document.getElementById("ReferenceTypeId").value;
    //刷新整棵树 因为BrowseName改了的话树要改
    d = new dTree('d');
    d.add(0,-1,'OPC UA NameSpace');
    d.add(1,0,'Objects',`javascript:displayNameSpace()`);
    d.add(2,0,'Types','','','',"img/folderopen.gif");
    d.add(3,0,'Views','','','',"img/folderopen.gif");
    nodeIndex = 4;
    for (var i=0;i<currentJSONModel.AddressSpace.length;i++)
    {
        creatModelTree(currentJSONModel.AddressSpace[i],1);
    }
    document.getElementById("devTree").innerHTML = '<div class="nodeAttrTitle">&nbsp;地址空间</div>';
    document.getElementById("devTree").innerHTML += d;
    d.openAll();
    if (thisNode.NodeClass == "Variable"){
        thisNode.DataType = document.getElementById("DataType").value;
        thisNode.InitialValue = document.getElementById("InitialValue").value;
        thisNode.BottomProtocolType = document.getElementById("BottomProtocolType").value;
        thisNode.MB_MachineAddress = document.getElementById("MB_MachineAddress").value;
        thisNode.MB_RegisterAddress = document.getElementById("MB_RegisterAddress").value;
        thisNode.MB_FunctionCode = document.getElementById("MB_FunctionCode").value;
        thisNode.MB_DataBitSize = document.getElementById("MB_DataBitSize").value;
        thisNode.RefreshFrequency = document.getElementById("RefreshFrequency").value;
        thisNode.BottomDataType = document.getElementById("BottomDataType").value;
        thisNode.MappingCoefficient = document.getElementById("MappingCoefficient").value;
    }
}
function deleteNode(nodeName){
    function RecurrenceDelete(childNodeArray){
        for (tempCNode in childNodeArray){
            if (childNodeArray[tempCNode].NodeID_ID == nodeName){
                childNodeArray.splice(tempCNode,1);
                return true;
            }
            if ("ChildNode" in childNodeArray[tempCNode]){
                if (RecurrenceDelete(childNodeArray[tempCNode].ChildNode)){
                    return true;
                }
            }
        }
    }
    for (tempNode in currentJSONModel.AddressSpace){
        if (currentJSONModel.AddressSpace[tempNode].NodeID_ID == nodeName){
            currentJSONModel.AddressSpace.splice(tempNode,1);
            displayNameSpace();
            break;
        }
        if ("ChildNode" in currentJSONModel.AddressSpace[tempNode]){
            if (RecurrenceDelete(currentJSONModel.AddressSpace[tempNode].ChildNode)){
                displayAttr(currentJSONModel.AddressSpace[tempNode].NodeID_ID);
                break;
            }
        }
    }
    //刷新整棵树
    d = new dTree('d');
    d.add(0,-1,'OPC UA NameSpace');
    d.add(1,0,'Objects',`javascript:displayNameSpace()`);
    d.add(2,0,'Types','','','',"img/folderopen.gif");
    d.add(3,0,'Views','','','',"img/folderopen.gif");
    nodeIndex = 4;
    for (var i=0;i<currentJSONModel.AddressSpace.length;i++)
    {
        creatModelTree(currentJSONModel.AddressSpace[i],1);
    }
    document.getElementById("devTree").innerHTML = '<div class="nodeAttrTitle">&nbsp;地址空间</div>';
    document.getElementById("devTree").innerHTML += d;
    d.openAll();
    // console.log(nodeName);
}
function nodeClassChange(data){
    if (data=='Variable'){//判断节点类型，
        document.getElementById("variableAttributes").style.display="block";
        document.getElementById("bottomProtocol").style.display="block";
    }else{
        document.getElementById("variableAttributes").style.display="none";
        document.getElementById("bottomProtocol").style.display="none";
    }
}
function addChildNode(){
    var tempName = document.getElementById("newNodeName").value;
    var tempNodeClass = document.getElementById("newNodeClass").value;
    if (checkNameisValidity(tempName)==false) {
        alert("结点名不可重名");
        console.log('err');
        return ;
    }
    if (currentDisplayNode==null) {
        console.log("Add node error");
    }
    var thisNode = getUANodeById(currentDisplayNode);
    if ("ChildNode" in thisNode){
        var childLength = thisNode.ChildNode.length;
        thisNode.ChildNode.push({});
        thisNode.ChildNode[childLength].NodeID_NamespaceIndex = "1";
        thisNode.ChildNode[childLength].NodeID_IDType = "String";
        thisNode.ChildNode[childLength].NodeID_ID = tempName;
        thisNode.ChildNode[childLength].NodeClass = tempNodeClass;
        thisNode.ChildNode[childLength].BrowseName = tempName;
        thisNode.ChildNode[childLength].DisplayName = tempName;
        thisNode.ChildNode[childLength].Description = tempName;
        thisNode.ChildNode[childLength].ReferenceTypeId = "Organizes";
        thisNode.ChildNode[childLength].QualifiedName = tempName;
    }else{
        thisNode.ChildNode = [];
        thisNode.ChildNode.push({});
        thisNode.ChildNode[0].NodeID_NamespaceIndex = "1";
        thisNode.ChildNode[0].NodeID_IDType = "String";
        thisNode.ChildNode[0].NodeID_ID = tempName;
        thisNode.ChildNode[0].NodeClass = tempNodeClass;
        thisNode.ChildNode[0].BrowseName = tempName;
        thisNode.ChildNode[0].DisplayName = tempName;
        thisNode.ChildNode[0].Description = tempName;
        thisNode.ChildNode[0].ReferenceTypeId = "Organizes";
        thisNode.ChildNode[0].QualifiedName = tempName;
    }
    //刷新整棵树
    d = new dTree('d');
    d.add(0,-1,'OPC UA NameSpace');
    d.add(1,0,'Objects',`javascript:displayNameSpace()`);
    d.add(2,0,'Types','','','',"img/folderopen.gif");
    d.add(3,0,'Views','','','',"img/folderopen.gif");
    nodeIndex = 4;
    for (var i=0;i<currentJSONModel.AddressSpace.length;i++)
    {
        creatModelTree(currentJSONModel.AddressSpace[i],1);
    }
    document.getElementById("devTree").innerHTML = '<div class="nodeAttrTitle">&nbsp;地址空间</div>';
    document.getElementById("devTree").innerHTML += d;
    d.openAll();
    displayAttr(currentDisplayNode);
}
function addNodeToASP(){
    var tempName = document.getElementById("newNodeName").value;
    var tempNodeClass = document.getElementById("newNodeClass").value;
    if (checkNameisValidity(tempName)==false) {
        alert("结点名不可重名");
        console.log('err');
        return ;
    }
    if ("AddressSpace" in currentJSONModel){
        var childLength = currentJSONModel.AddressSpace.length;
        currentJSONModel.AddressSpace.push({});
        currentJSONModel.AddressSpace[childLength].NodeID_NamespaceIndex = "1";
        currentJSONModel.AddressSpace[childLength].NodeID_IDType = "String";
        currentJSONModel.AddressSpace[childLength].NodeID_ID = tempName;
        currentJSONModel.AddressSpace[childLength].NodeClass = tempNodeClass;
        currentJSONModel.AddressSpace[childLength].BrowseName = tempName;
        currentJSONModel.AddressSpace[childLength].DisplayName = tempName;
        currentJSONModel.AddressSpace[childLength].Description = tempName;
        currentJSONModel.AddressSpace[childLength].ReferenceTypeId = "Organizes";
        currentJSONModel.AddressSpace[childLength].QualifiedName = tempName;
    }else{
        currentJSONModel.AddressSpace = [];
        currentJSONModel.AddressSpace.push({});
        currentJSONModel.AddressSpace[0].NodeID_NamespaceIndex = "1";
        currentJSONModel.AddressSpace[0].NodeID_IDType = "String";
        currentJSONModel.AddressSpace[0].NodeID_ID = tempName;
        currentJSONModel.AddressSpace[0].NodeClass = tempNodeClass;
        currentJSONModel.AddressSpace[0].BrowseName = tempName;
        currentJSONModel.AddressSpace[0].DisplayName = tempName;
        currentJSONModel.AddressSpace[0].Description = tempName;
        currentJSONModel.AddressSpace[0].ReferenceTypeId = "Organizes";
        currentJSONModel.AddressSpace[0].QualifiedName = tempName;
    }
    //刷新整棵树
    d = new dTree('d');
    d.add(0,-1,'OPC UA NameSpace');
    d.add(1,0,'Objects',`javascript:displayNameSpace()`);
    d.add(2,0,'Types','','','',"img/folderopen.gif");
    d.add(3,0,'Views','','','',"img/folderopen.gif");
    nodeIndex = 4;
    for (var i=0;i<currentJSONModel.AddressSpace.length;i++)
    {
        creatModelTree(currentJSONModel.AddressSpace[i],1);
    }
    document.getElementById("devTree").innerHTML = '<div class="nodeAttrTitle">&nbsp;地址空间</div>';
    document.getElementById("devTree").innerHTML += d;
    d.openAll();
    displayNameSpace();
}
//返回：true-合法 false-不合法
function checkNameisValidity(nameID){
    //返回 查找到名字返回名字，否则null
    function RecurrenceFindName(node){
        if (node.NodeID_ID == nameID) return nameID;
        if ("ChildNode" in node){
            for (tempIndex in node.ChildNode){
                if (RecurrenceFindName(node.ChildNode[tempIndex]) != null) return nameID;
            }
        }
        return null;
    }
    for (tempIndex in currentJSONModel.AddressSpace){
        if (RecurrenceFindName(currentJSONModel.AddressSpace[tempIndex]) != null) return false;
    }
    return true;
}
function saveCurrentConfigFile()
{
    if (currentDisplayNode!=null) {
        saveAttrToVar(currentDisplayNode);
    }
    currentJSONModel.OldName = currentJSONModel.Name;
    currentJSONModel.Name = document.getElementById('configFileName').value;
    currentJSONModel.Author = document.getElementById('configFileAuthor').value;
    currentJSONModel.Date = document.getElementById('configFileDate').value;
    currentJSONModel.Summary = document.getElementById('configFileSummary').value;
    xmlhttplist=new XMLHttpRequest();
    xmlhttplist.onreadystatechange = function(){
        if (xmlhttplist.readyState!=4 || xmlhttplist.status!=200) return;
        console.log(xmlhttplist.responseText);
        loadCurrentModle();
    }
    xmlhttplist.open("POST","dealModel.php",true);
    // xmlhttplist.setRequestHeader("Content-Length",JSON.stringify(currentJSONModel).lenght);
    xmlhttplist.setRequestHeader("Content-type","application/json;charset-UTF-8");
    xmlhttplist.send(JSON.stringify(currentJSONModel));
}
function createNewConfigFile(){
    xmlhttplist=new XMLHttpRequest();
    xmlhttplist.onreadystatechange = function(){
        if (xmlhttplist.readyState!=4 || xmlhttplist.status!=200) return;
        loadCurrentModle();
    }
    xmlhttplist.open("GET",`dealModel.php?cmd=createNewConfigFile`,true);
    xmlhttplist.send();
}
function deleteConfigFile(){
    var queren = confirm("删除是不可恢复的，你确认要删除吗？");
    if (queren==false) return;
    xmlhttplist=new XMLHttpRequest();
    xmlhttplist.onreadystatechange = function(){
        if (xmlhttplist.readyState!=4 || xmlhttplist.status!=200) return;
        loadCurrentModle();
    };
    xmlhttplist.open("GET",`dealModel.php?cmd=deleteConfigFile&name=${currentJSONModel.Name}`,true);
    xmlhttplist.send();
}
function useCurrentConfigFile(){
    xmlhttplist=new XMLHttpRequest();
    xmlhttplist.onreadystatechange = function(){
        if (xmlhttplist.readyState!=4 || xmlhttplist.status!=200) return;
        loadCurrentModle();
    };
    xmlhttplist.open("GET",`dealModel.php?cmd=useCurrentConfigFile&name=${currentJSONModel.Name}`,true);
    xmlhttplist.send();
}