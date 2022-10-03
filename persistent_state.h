struct PersistentState
{
    String wifiSSID;
    String wifiPwd;
    String token;

    void Reset()
    {
        wifiSSID = "";
        wifiPwd = "";
        token = "";
    }
};
