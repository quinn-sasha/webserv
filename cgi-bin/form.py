import os
import sys

print("Content-Type: text/html\r")
print("\r")
print("""<html>
<head>
<title>CGI Form Test</title>
<style>
body {
  font-family: Arial, sans-serif;
  max-width: 600px;
  margin: 50px auto;
  padding: 20px;
  background-color: #f5f5f5;
}
form {
  background-color: white;
  padding: 30px;
  border-radius: 8px;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}
label {
  display: block;
  margin-bottom: 5px;
  font-weight: bold;
}
input[type="text"] {
  width: 100%;
  padding: 8px;
  margin-bottom: 15px;
  border: 1px solid #ddd;
  border-radius: 4px;
  box-sizing: border-box;
}
button {
  background-color: #4CAF50;
  color: white;
  padding: 10px 20px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 16px;
}
button:hover {
  background-color: #45a049;
}
</style>
</head>
<body>
<h1>🚀 CGI Form Test</h1>
<form method="POST" action="/cgi-bin/test.py">
  <label for="name">Name:</label>
  <input type="text" id="name" name="name" value="John" required>
  
  <label for="age">Age:</label>
  <input type="text" id="age" name="age" value="30" required>
  
  <button type="submit">Submit</button>
</form>
</body>
</html>
""")