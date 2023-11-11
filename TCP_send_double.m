%address = "192.168.1.24"; % use this for wifi
address = "192.168.7.2"; % use this for wifi

t = tcpclient(address,1045);

for x = 0:0.1:2*pi  % Change the range and step size as needed
    y = sin(x);
    write(t, y);
    pause(1);  % Adjust the pause duration (in seconds) as needed
end



