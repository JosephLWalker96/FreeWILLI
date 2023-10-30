address = "192.168.1.24";
t = tcpclient(address,1045);

for x = 0:0.1:2*pi  % Change the range and step size as needed
    y = sin(x);
    write(t, y);
    pause(2);  % Adjust the pause duration (in seconds) as needed
end

