 window.addEventListener("load", function() {
     document.getElementById("searchDb").addEventListener("click", searchDatabase, false);
     document.getElementById("insertDb").addEventListener("click", insertToDatabase, false);
     document.getElementById("updateDb").addEventListener("click", updateDatabase, false);
     document.getElementById("deleteDb").addEventListener("click", deleteFromDatabase, false);
 });

 function generateTable(xml) {
     var xml_response = xml.responseText;
     var emptyReplyLength = ("<phonebook></phonebook>").length;

     if (xml_response.length > emptyReplyLength) {
         var parser = new DOMParser();
         var doc = parser.parseFromString(xml_response, "application/xml");

         var table = "<tr><th>Name</th><th>Phone</th><th>ID</th></tr>";
         var elem = doc.getElementsByTagName("contact");

         var name;

         for (var i = 0; i < elem.length; i++) {
             name = elem[i].getElementsByTagName("name")[0].childNodes[0];

             !name ? name = "" : name = elem[i].getElementsByTagName("name")[0].childNodes[0].nodeValue;

             table += "<tr><td>" +
                 name + "</td><td>" +
                 elem[i].getElementsByTagName("tlf")[0].childNodes[0].nodeValue + "</td><td>" +
                 elem[i].getElementsByTagName("id")[0].childNodes[0].nodeValue + "</td></tr>";
         }
         document.getElementById("showTable").innerHTML = table;
         document.getElementById("response").innerHTML = "Output:";
     } else {
         document.getElementById("response").innerHTML = "Server unavailable. Please try again later.";
     }
 }

 function searchDatabase() {

 }

 function insertToDatabase() {
     var id = document.getElementsByName("id")[0].value;
     var tlf = document.getElementsByName("tlf")[0].value;
     var name = document.getElementsByName("name")[0].value;

     if (!id || !tlf) {
         alert(id ? (tlf ? exit(0) : "Phone number can't be empty!") : (tlf ? "ID can't be empty!" : "Phone number and ID can't be empty!"));
     }

     var add_xml = "<phonebook><contact><name>" + name + "</name><tlf>" + tlf + "</tlf><id>" + id + "</id></contact></phonebook>";

     xml_request = new XMLHttpRequest();

     xml_request.open("POST", "/", true);
     xml_request.send(add_xml);

     xml_request.onreadystatechange = function() {
         if (xml_request.readyState == XMLHttpRequest.DONE) {
             document.getElementById("response").innerHTML = xml_request.responseText;

             /*if (!xml_request.responseText) {
                 document.getElementById("response").innerHTML = "Error: empty response";
             }*/
         }
     }
 }

 function updateDatabase() {

 }

 function deleteFromDatabase() {

 }